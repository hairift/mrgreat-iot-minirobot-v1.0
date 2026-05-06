#include "mqtt_protocol.h"
#include "board.h"
#include "application.h"
#include "settings.h"

#include <esp_log.h>
#include <cstring>
#include <cctype>
#include <arpa/inet.h>
#include "assets/lang_config.h"

#define TAG "MQTT"

MqttProtocol::MqttProtocol() {
    event_group_handle_ = xEventGroupCreate();
    mbedtls_aes_init(&aes_ctx_);

    // Inisialisasi pewaktu sambung ulang
    esp_timer_create_args_t reconnect_timer_args = {
        .callback = [](void* arg) {
            MqttProtocol* protocol = (MqttProtocol*)arg;
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateIdle) {
                ESP_LOGI(TAG, "Menghubungkan kembali ke server MQTT");
                auto alive = protocol->alive_;  // Simpan status hidup saat ini
                app.Schedule([protocol, alive]() {
                    if (*alive) {
                        protocol->StartMqttClient(false);
                    }
                });
            }
        },
        .arg = this,
    };
    esp_timer_create(&reconnect_timer_args, &reconnect_timer_);
}

MqttProtocol::~MqttProtocol() {
    ESP_LOGI(TAG, "MqttProtocol deinit");
    
    // Tandai sebagai mati terlebih dahulu untuk mencegah tugas terjadwal yang tertunda dari eksekusi
    *alive_ = false;
    
    if (reconnect_timer_ != nullptr) {
        esp_timer_stop(reconnect_timer_);
        esp_timer_delete(reconnect_timer_);
    }

    udp_.reset();
    mqtt_.reset();
    mbedtls_aes_free(&aes_ctx_);
    
    if (event_group_handle_ != nullptr) {
        vEventGroupDelete(event_group_handle_);
    }
}

bool MqttProtocol::Start() {
    return StartMqttClient(false);
}

bool MqttProtocol::StartMqttClient(bool report_error) {
    if (mqtt_ != nullptr) {
        ESP_LOGW(TAG, "Client MQTT sudah dimulai");
        mqtt_.reset();
    }

    Settings settings("mqtt", false);
    auto endpoint = settings.GetString("endpoint");
    auto client_id = settings.GetString("client_id");
    auto username = settings.GetString("username");
    auto password = settings.GetString("password");
    int keepalive_interval = settings.GetInt("keepalive", 240);
    publish_topic_ = settings.GetString("publish_topic");

    if (endpoint.empty()) {
        ESP_LOGW(TAG, "Endpoint MQTT tidak ditentukan");
        if (report_error) {
            SetError(Lang::Strings::SERVER_NOT_FOUND);
        }
        return false;
    }

    auto network = Board::GetInstance().GetNetwork();
    if (network == nullptr) {
        ESP_LOGE(TAG, "Network interface tidak tersedia");
        return false;
    }
    mqtt_ = network->CreateMqtt(0);
    if (mqtt_ == nullptr) {
        ESP_LOGE(TAG, "Gagal membuat client MQTT");
        return false;
    }
    mqtt_->SetKeepAlive(keepalive_interval);

    mqtt_->OnDisconnected([this]() {
        if (on_disconnected_ != nullptr) {
            on_disconnected_();
        }
        ESP_LOGI(TAG, "MQTT terputus, jadwalkan reconnect dalam %d detik", MQTT_RECONNECT_INTERVAL_MS / 1000);
        esp_timer_start_once(reconnect_timer_, MQTT_RECONNECT_INTERVAL_MS * 1000);
    });

    mqtt_->OnConnected([this]() {
        if (on_connected_ != nullptr) {
            on_connected_();
        }
        esp_timer_stop(reconnect_timer_);
    });

    mqtt_->OnMessage([this](const std::string& topic, const std::string& payload) {
        cJSON* root = cJSON_Parse(payload.c_str());
        if (root == nullptr) {
            ESP_LOGE(TAG, "Gagal mengurai pesan json %s", payload.c_str());
            return;
        }
        cJSON* type = cJSON_GetObjectItem(root, "type");
        if (!cJSON_IsString(type)) {
            ESP_LOGE(TAG, "Tipe pesan tidak valid");
            cJSON_Delete(root);
            return;
        }

        if (strcmp(type->valuestring, "hello") == 0) {
            ParseServerHello(root);
        } else if (strcmp(type->valuestring, "goodbye") == 0) {
            auto session_id = cJSON_GetObjectItem(root, "session_id");
            const char* received_session_id = cJSON_IsString(session_id) ? session_id->valuestring : nullptr;
            ESP_LOGI(TAG, "Menerima pesan goodbye, session_id: %s", received_session_id ? received_session_id : "null");
            if (received_session_id == nullptr || session_id_ == received_session_id) {
                auto alive = alive_;  // Simpan status hidup saat ini
                Application::GetInstance().Schedule([this, alive]() {
                    if (*alive) {
                        // Server memulai goodbye, jangan kirim goodbye balik untuk menghindari ping-pong
                        CloseAudioChannel(false);
                    }
                });
            }
        } else if (on_incoming_json_ != nullptr) {
            on_incoming_json_(root);
        }
        cJSON_Delete(root);
        last_incoming_time_ = std::chrono::steady_clock::now();
    });

    ESP_LOGI(TAG, "Menghubungkan ke endpoint %s", endpoint.c_str());
    std::string broker_address;
    int broker_port = 8883;
    size_t pos = endpoint.find(':');
    if (pos != std::string::npos) {
        broker_address = endpoint.substr(0, pos);
        try {
            broker_port = std::stoi(endpoint.substr(pos + 1));
        } catch (const std::exception&) {
            ESP_LOGE(TAG, "Port MQTT tidak valid: %s", endpoint.c_str());
            SetError(Lang::Strings::SERVER_NOT_FOUND);
            return false;
        }
    } else {
        broker_address = endpoint;
    }
    if (!mqtt_->Connect(broker_address, broker_port, client_id, username, password)) {
        ESP_LOGE(TAG, "Gagal terhubung ke endpoint, code=%d", mqtt_->GetLastError());
        SetError(Lang::Strings::SERVER_NOT_CONNECTED);
        return false;
    }

    ESP_LOGI(TAG, "Terhubung ke endpoint");
    return true;
}

bool MqttProtocol::SendText(const std::string& text) {
    if (publish_topic_.empty()) {
        return false;
    }
    if (mqtt_ == nullptr || !mqtt_->IsConnected()) {
        ESP_LOGW(TAG, "Klien MQTT tidak terhubung");
        return false;
    }
    if (!mqtt_->Publish(publish_topic_, text)) {
        ESP_LOGE(TAG, "Gagal mempublikasikan pesan, panjang payload=%u byte",
                 static_cast<unsigned>(text.size()));
        SetError(Lang::Strings::SERVER_ERROR);
        return false;
    }
    return true;
}

bool MqttProtocol::SendAudio(std::unique_ptr<AudioStreamPacket> packet) {
    std::lock_guard<std::mutex> lock(channel_mutex_);
    if (udp_ == nullptr) {
        return false;
    }
    if (aes_nonce_.size() < 16) {
        ESP_LOGE(TAG, "AES nonce belum valid");
        return false;
    }
    if (packet->payload.size() > UINT16_MAX) {
        ESP_LOGE(TAG, "Payload audio terlalu besar: %u", packet->payload.size());
        return false;
    }

    std::string nonce(aes_nonce_);
    uint16_t payload_size_be = htons(static_cast<uint16_t>(packet->payload.size()));
    uint32_t timestamp_be = htonl(packet->timestamp);
    uint32_t sequence_be = htonl(++local_sequence_);
    memcpy(&nonce[2], &payload_size_be, sizeof(payload_size_be));
    memcpy(&nonce[8], &timestamp_be, sizeof(timestamp_be));
    memcpy(&nonce[12], &sequence_be, sizeof(sequence_be));

    std::string encrypted;
    encrypted.resize(aes_nonce_.size() + packet->payload.size());
    memcpy(encrypted.data(), nonce.data(), nonce.size());

    size_t nc_off = 0;
    uint8_t stream_block[16] = {0};
    if (mbedtls_aes_crypt_ctr(&aes_ctx_, packet->payload.size(), &nc_off,
        reinterpret_cast<unsigned char*>(nonce.data()), stream_block,
        reinterpret_cast<const unsigned char*>(packet->payload.data()),
        reinterpret_cast<unsigned char*>(&encrypted[nonce.size()])) != 0) {
        ESP_LOGE(TAG, "Gagal mengenkripsi data audio");
        return false;
    }

    return udp_->Send(encrypted) > 0;
}

void MqttProtocol::CloseAudioChannel(bool send_goodbye) {
    {
        std::lock_guard<std::mutex> lock(channel_mutex_);
        udp_.reset();
    }

    ESP_LOGI(TAG, "Menutup channel audio, send_goodbye: %d", send_goodbye);

    // Hanya kirim goodbye saat client memulai penutupan
    // Jangan kirim jika server sudah mengirim goodbye (untuk menghindari ping-pong)
    if (send_goodbye) {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "session_id", session_id_.c_str());
        cJSON_AddStringToObject(root, "type", "goodbye");
        char* json_str = cJSON_PrintUnformatted(root);
        std::string message = json_str ? json_str : "{}";
        if (json_str != nullptr) {
            cJSON_free(json_str);
        }
        cJSON_Delete(root);
        SendText(message);
    }

    if (on_audio_channel_closed_ != nullptr) {
        on_audio_channel_closed_();
    }
}

bool MqttProtocol::OpenAudioChannel() {
    if (mqtt_ == nullptr || !mqtt_->IsConnected()) {
        ESP_LOGI(TAG, "MQTT tidak terhubung, coba hubungkan sekarang");
        if (!StartMqttClient(true)) {
            return false;
        }
    }

    error_occurred_ = false;
    session_id_ = "";
    last_incoming_time_ = std::chrono::steady_clock::now();
    xEventGroupClearBits(event_group_handle_, MQTT_PROTOCOL_SERVER_HELLO_EVENT);

    auto message = GetHelloMessage();
    if (!SendText(message)) {
        return false;
    }

    // Tunggu respons server
    EventBits_t bits = xEventGroupWaitBits(event_group_handle_, MQTT_PROTOCOL_SERVER_HELLO_EVENT, pdTRUE, pdFALSE, pdMS_TO_TICKS(10000));
    if (!(bits & MQTT_PROTOCOL_SERVER_HELLO_EVENT)) {
        ESP_LOGE(TAG, "Gagal menerima server hello");
        SetError(Lang::Strings::SERVER_TIMEOUT);
        return false;
    }

    std::lock_guard<std::mutex> lock(channel_mutex_);
    auto network = Board::GetInstance().GetNetwork();
    if (network == nullptr) {
        ESP_LOGE(TAG, "Network interface tidak tersedia untuk UDP");
        return false;
    }
    udp_ = network->CreateUdp(2);
    if (udp_ == nullptr) {
        ESP_LOGE(TAG, "Gagal membuat socket UDP");
        SetError(Lang::Strings::SERVER_ERROR);
        return false;
    }
    udp_->OnMessage([this](const std::string& data) {
        /*
         * Format paket OPUS terenkripsi melalui UDP:
         * |type 1u|flags 1u|payload_len 2u|ssrc 4u|timestamp 4u|sequence 4u|
         * |payload payload_len|
         */
        if (data.size() < aes_nonce_.size() || aes_nonce_.size() < 16) {
            ESP_LOGE(TAG, "Ukuran paket audio tidak valid: %u", data.size());
            return;
        }
        if (data[0] != 0x01) {
            ESP_LOGE(TAG, "Tipe paket audio tidak valid: %x", data[0]);
            return;
        }
        uint16_t payload_size_be = 0;
        uint32_t timestamp_be = 0;
        uint32_t sequence_be = 0;
        memcpy(&payload_size_be, data.data() + 2, sizeof(payload_size_be));
        memcpy(&timestamp_be, data.data() + 8, sizeof(timestamp_be));
        memcpy(&sequence_be, data.data() + 12, sizeof(sequence_be));
        uint16_t declared_payload_size = ntohs(payload_size_be);
        size_t decrypted_size = data.size() - aes_nonce_.size();
        if (declared_payload_size != decrypted_size) {
            ESP_LOGW(TAG, "Ukuran payload audio tidak cocok: header=%u aktual=%u",
                declared_payload_size, static_cast<unsigned>(decrypted_size));
            return;
        }
        uint32_t timestamp = ntohl(timestamp_be);
        uint32_t sequence = ntohl(sequence_be);
        if (sequence < remote_sequence_) {
            int64_t now_us = esp_timer_get_time();
            if (now_us - last_sequence_warning_us_ > 1000000) {
                last_sequence_warning_us_ = now_us;
                ESP_LOGW(TAG, "Menerima paket audio dengan sequence lama: %lu, yang diharapkan: %lu", sequence, remote_sequence_);
            }
            return;
        }
        if (sequence != remote_sequence_ + 1) {
            int64_t now_us = esp_timer_get_time();
            if (now_us - last_sequence_warning_us_ > 1000000) {
                last_sequence_warning_us_ = now_us;
                ESP_LOGW(TAG, "Menerima paket audio dengan sequence salah: %lu, yang diharapkan: %lu", sequence, remote_sequence_ + 1);
            }
        }

        size_t nc_off = 0;
        uint8_t stream_block[16] = {0};
        std::string nonce(data.data(), aes_nonce_.size());
        auto encrypted = reinterpret_cast<const unsigned char*>(data.data() + aes_nonce_.size());
        auto packet = std::make_unique<AudioStreamPacket>();
        packet->sample_rate = server_sample_rate_;
        packet->frame_duration = server_frame_duration_;
        packet->timestamp = timestamp;
        packet->payload.resize(decrypted_size);
        int ret = mbedtls_aes_crypt_ctr(&aes_ctx_, decrypted_size, &nc_off,
            reinterpret_cast<unsigned char*>(nonce.data()), stream_block, encrypted,
            reinterpret_cast<unsigned char*>(packet->payload.data()));
        if (ret != 0) {
                ESP_LOGE(TAG, "Gagal mendekripsi data audio, ret: %d", ret);
            return;
        }
        if (on_incoming_audio_ != nullptr) {
            on_incoming_audio_(std::move(packet));
        }
        remote_sequence_ = sequence;
        last_incoming_time_ = std::chrono::steady_clock::now();
    });

    udp_->Connect(udp_server_, udp_port_);

    if (on_audio_channel_opened_ != nullptr) {
        on_audio_channel_opened_();
    }
    return true;
}

std::string MqttProtocol::GetHelloMessage() {
    // Kirim pesan hello untuk meminta kanal UDP
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "hello");
    cJSON_AddNumberToObject(root, "version", 3);
    cJSON_AddStringToObject(root, "transport", "udp");
    cJSON* features = cJSON_CreateObject();
#if CONFIG_USE_SERVER_AEC
    cJSON_AddBoolToObject(features, "aec", true);
#endif
    cJSON_AddBoolToObject(features, "mcp", true);
    cJSON_AddItemToObject(root, "features", features);
    cJSON* audio_params = cJSON_CreateObject();
    cJSON_AddStringToObject(audio_params, "format", "opus");
    cJSON_AddNumberToObject(audio_params, "sample_rate", 16000);
    cJSON_AddNumberToObject(audio_params, "channels", 1);
    cJSON_AddNumberToObject(audio_params, "frame_duration", OPUS_FRAME_DURATION_MS);
    cJSON_AddItemToObject(root, "audio_params", audio_params);
    auto json_str = cJSON_PrintUnformatted(root);
    std::string message(json_str);
    cJSON_free(json_str);
    cJSON_Delete(root);
    return message;
}

void MqttProtocol::ParseServerHello(const cJSON* root) {
    auto transport = cJSON_GetObjectItem(root, "transport");
    if (!cJSON_IsString(transport) || strcmp(transport->valuestring, "udp") != 0) {
        ESP_LOGE(TAG, "Transport tidak didukung: %s", cJSON_IsString(transport) ? transport->valuestring : "(invalid)");
        return;
    }

    auto session_id = cJSON_GetObjectItem(root, "session_id");
    if (cJSON_IsString(session_id)) {
        session_id_ = session_id->valuestring;
        ESP_LOGI(TAG, "ID Sesi: %s", session_id_.c_str());
    }

    // Dapatkan sample rate dari pesan hello
    auto audio_params = cJSON_GetObjectItem(root, "audio_params");
    if (cJSON_IsObject(audio_params)) {
        auto sample_rate = cJSON_GetObjectItem(audio_params, "sample_rate");
        if (cJSON_IsNumber(sample_rate)) {
            server_sample_rate_ = sample_rate->valueint;
        }
        auto frame_duration = cJSON_GetObjectItem(audio_params, "frame_duration");
        if (cJSON_IsNumber(frame_duration)) {
            server_frame_duration_ = frame_duration->valueint;
        }
    }

    auto udp = cJSON_GetObjectItem(root, "udp");
    if (!cJSON_IsObject(udp)) {
        ESP_LOGE(TAG, "UDP tidak ditentukan");
        return;
    }
    auto server = cJSON_GetObjectItem(udp, "server");
    auto port = cJSON_GetObjectItem(udp, "port");
    auto key = cJSON_GetObjectItem(udp, "key");
    auto nonce = cJSON_GetObjectItem(udp, "nonce");
    if (!cJSON_IsString(server) || !cJSON_IsNumber(port) || !cJSON_IsString(key) || !cJSON_IsString(nonce)) {
        ESP_LOGE(TAG, "UDP config tidak lengkap atau tidak valid");
        return;
    }
    if (port->valueint <= 0 || port->valueint > 65535) {
        ESP_LOGE(TAG, "UDP port tidak valid: %d", port->valueint);
        return;
    }

    // auto encryption = cJSON_GetObjectItem(udp, "encryption")->valuestring;
    // ESP_LOGI(TAG, "UDP server: %s, port: %d, encryption: %s", udp_server_.c_str(), udp_port_, encryption);
    std::string decoded_key = DecodeHexString(key->valuestring);
    aes_nonce_ = DecodeHexString(nonce->valuestring);
    if (decoded_key.size() != 16 || aes_nonce_.size() < 16) {
        ESP_LOGE(TAG, "UDP crypto key/nonce tidak valid");
        return;
    }

    udp_server_ = server->valuestring;
    udp_port_ = port->valueint;
    mbedtls_aes_free(&aes_ctx_);
    mbedtls_aes_init(&aes_ctx_);
    if (mbedtls_aes_setkey_enc(&aes_ctx_, reinterpret_cast<const unsigned char*>(decoded_key.data()), decoded_key.size() * 8) != 0) {
        ESP_LOGE(TAG, "Gagal menginisialisasi AES context");
        return;
    }
    local_sequence_ = 0;
    remote_sequence_ = 0;
    last_sequence_warning_us_ = 0;
    xEventGroupSetBits(event_group_handle_, MQTT_PROTOCOL_SERVER_HELLO_EVENT);
}

static const char hex_chars[] = "0123456789ABCDEF";
// Fungsi pembantu, mengonversi karakter heksadesimal tunggal ke nilai numerik yang sesuai
static inline uint8_t CharToHex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;  // Untuk input tidak valid, kembalikan 0
}

std::string MqttProtocol::DecodeHexString(const std::string& hex_string) {
    if (hex_string.size() % 2 != 0) {
        return "";
    }

    std::string decoded;
    decoded.reserve(hex_string.size() / 2);
    for (size_t i = 0; i < hex_string.size(); i += 2) {
        if (!std::isxdigit(static_cast<unsigned char>(hex_string[i])) ||
            !std::isxdigit(static_cast<unsigned char>(hex_string[i + 1]))) {
            return "";
        }
        char byte = (CharToHex(hex_string[i]) << 4) | CharToHex(hex_string[i + 1]);
        decoded.push_back(byte);
    }
    return decoded;
}

bool MqttProtocol::IsAudioChannelOpened() const {
    return udp_ != nullptr && !error_occurred_ && !IsTimeout();
}
