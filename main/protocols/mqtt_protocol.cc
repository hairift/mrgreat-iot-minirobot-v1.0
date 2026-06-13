#include "mqtt_protocol.h"
#include "board.h"
#include "application.h"
#include "settings.h"

#include <esp_log.h>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <limits>
#include <cstring>
#include <arpa/inet.h>
#include "assets/lang_config.h"

#define TAG "MQTT"

namespace {

constexpr int kServerHelloTimeoutMs = 10000;
constexpr size_t kAesNonceSize = 16;
constexpr int64_t kSequenceWarningIntervalUs = 1000000;

}  // namespace

MqttProtocol::MqttProtocol() {
    event_group_handle_ = xEventGroupCreate();
    mbedtls_aes_init(&aes_ctx_);

    // Siapkan pewaktu untuk mencoba kembali MQTT tanpa menahan tugas utama.
    esp_timer_create_args_t reconnect_timer_args = {
        .callback = [](void* arg) {
            MqttProtocol* protocol = (MqttProtocol*)arg;
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateIdle) {
                ESP_LOGI(TAG, "Reconnecting to MQTT server");
                auto alive = protocol->alive_;
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
    
    // Tandai objek tidak aktif sebelum membatalkan pekerjaan terjadwal.
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
        ESP_LOGW(TAG, "Mqtt client already started");
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
        ESP_LOGW(TAG, "MQTT endpoint is not specified");
        if (report_error) {
            SetError(Lang::Strings::SERVER_NOT_FOUND);
        }
        return false;
    }

    auto network = Board::GetInstance().GetNetwork();
    if (network == nullptr) {
        ESP_LOGE(TAG, "Antarmuka jaringan tidak tersedia");
        if (report_error) {
            SetError(Lang::Strings::SERVER_NOT_CONNECTED);
        }
        return false;
    }
    mqtt_ = network->CreateMqtt(0);
    if (mqtt_ == nullptr) {
        ESP_LOGE(TAG, "Gagal membuat klien MQTT");
        if (report_error) {
            SetError(Lang::Strings::SERVER_ERROR);
        }
        return false;
    }
    mqtt_->SetKeepAlive(keepalive_interval);

    mqtt_->OnDisconnected([this]() {
        if (on_disconnected_ != nullptr) {
            on_disconnected_();
        }
        ESP_LOGI(TAG, "MQTT disconnected, schedule reconnect in %d seconds", MQTT_RECONNECT_INTERVAL_MS / 1000);
        if (esp_timer_is_active(reconnect_timer_)) {
            esp_timer_stop(reconnect_timer_);
        }
        esp_timer_start_once(reconnect_timer_, MQTT_RECONNECT_INTERVAL_MS * 1000);
    });

    mqtt_->OnConnected([this]() {
        if (on_connected_ != nullptr) {
            on_connected_();
        }
        if (esp_timer_is_active(reconnect_timer_)) {
            esp_timer_stop(reconnect_timer_);
        }
    });

    mqtt_->OnMessage([this](const std::string& topic, const std::string& payload) {
        cJSON* root = cJSON_Parse(payload.c_str());
        if (root == nullptr) {
            ESP_LOGE(TAG, "Failed to parse json message %s", payload.c_str());
            return;
        }
        cJSON* type = cJSON_GetObjectItem(root, "type");
        if (!cJSON_IsString(type)) {
            ESP_LOGE(TAG, "Message type is invalid");
            cJSON_Delete(root);
            return;
        }

        if (strcmp(type->valuestring, "hello") == 0) {
            ParseServerHello(root);
        } else if (strcmp(type->valuestring, "goodbye") == 0) {
            auto session_id = cJSON_GetObjectItem(root, "session_id");
            ESP_LOGI(TAG, "Menerima goodbye, session_id: %s",
                cJSON_IsString(session_id) ? session_id->valuestring : "tidak valid");
            if (cJSON_IsString(session_id) && !session_id_.empty() &&
                session_id_ == session_id->valuestring) {
                auto alive = alive_;
                Application::GetInstance().Schedule([this, alive]() {
                    if (*alive) {
                        // Server memulai penutupan, jadi jangan mengirim goodbye balasan.
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

    ESP_LOGI(TAG, "Connecting to endpoint %s", endpoint.c_str());
    std::string broker_address;
    int broker_port = 8883;
    size_t pos = endpoint.find(':');
    if (pos != std::string::npos) {
        broker_address = endpoint.substr(0, pos);
        const std::string port_text = endpoint.substr(pos + 1);
        char* end = nullptr;
        errno = 0;
        long parsed_port = std::strtol(port_text.c_str(), &end, 10);
        if (errno != 0 || end == port_text.c_str() || *end != '\0' ||
            parsed_port <= 0 || parsed_port > 65535) {
            ESP_LOGE(TAG, "Port MQTT tidak valid pada endpoint %s", endpoint.c_str());
            if (report_error) {
                SetError(Lang::Strings::SERVER_NOT_FOUND);
            }
            return false;
        }
        broker_port = static_cast<int>(parsed_port);
    } else {
        broker_address = endpoint;
    }
    if (!mqtt_->Connect(broker_address, broker_port, client_id, username, password)) {
        ESP_LOGE(TAG, "Failed to connect to endpoint, code=%d", mqtt_->GetLastError());
        if (report_error) {
            SetError(Lang::Strings::SERVER_NOT_CONNECTED);
        }
        return false;
    }

    ESP_LOGI(TAG, "Connected to endpoint");
    return true;
}

bool MqttProtocol::SendText(const std::string& text) {
    return PublishText(text, true);
}

bool MqttProtocol::PublishText(const std::string& text, bool report_error) {
    if (publish_topic_.empty()) {
        return false;
    }
    if (mqtt_ == nullptr || !mqtt_->IsConnected() || !mqtt_->Publish(publish_topic_, text)) {
        ESP_LOGE(TAG, "Gagal mempublikasikan pesan MQTT sepanjang %u byte",
            static_cast<unsigned>(text.size()));
        if (report_error) {
            SetError(Lang::Strings::SERVER_ERROR);
        }
        return false;
    }
    return true;
}

bool MqttProtocol::SendAudio(std::unique_ptr<AudioStreamPacket> packet) {
    std::lock_guard<std::mutex> lock(channel_mutex_);
    if (udp_ == nullptr) {
        return false;
    }
    if (aes_nonce_.size() != kAesNonceSize) {
        ESP_LOGE(TAG, "Nonce AES kanal audio tidak valid");
        return false;
    }
    if (packet == nullptr ||
        packet->payload.size() > std::numeric_limits<uint16_t>::max()) {
        ESP_LOGE(TAG, "Payload audio keluar dari batas protokol");
        return false;
    }

    std::string nonce(aes_nonce_);
    const uint16_t payload_size_be =
        htons(static_cast<uint16_t>(packet->payload.size()));
    const uint32_t timestamp_be = htonl(packet->timestamp);
    const uint32_t sequence_be = htonl(++local_sequence_);
    memcpy(nonce.data() + 2, &payload_size_be, sizeof(payload_size_be));
    memcpy(nonce.data() + 8, &timestamp_be, sizeof(timestamp_be));
    memcpy(nonce.data() + 12, &sequence_be, sizeof(sequence_be));

    std::string encrypted;
    encrypted.resize(aes_nonce_.size() + packet->payload.size());
    memcpy(encrypted.data(), nonce.data(), nonce.size());

    size_t nc_off = 0;
    uint8_t stream_block[16] = {0};
    if (mbedtls_aes_crypt_ctr(
            &aes_ctx_, packet->payload.size(), &nc_off,
            reinterpret_cast<unsigned char*>(nonce.data()), stream_block,
            reinterpret_cast<const unsigned char*>(packet->payload.data()),
            reinterpret_cast<unsigned char*>(encrypted.data() + nonce.size())) != 0) {
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

    ESP_LOGI(TAG, "Menutup kanal audio, kirim goodbye: %d", send_goodbye);

    // Hanya kirim goodbye ketika perangkat yang memulai penutupan.
    if (send_goodbye && !session_id_.empty()) {
        std::string message = "{";
        message += "\"session_id\":\"" + session_id_ + "\",";
        message += "\"type\":\"goodbye\"";
        message += "}";
        SendText(message);
    }

    if (on_audio_channel_closed_ != nullptr) {
        on_audio_channel_closed_();
    }
}

bool MqttProtocol::OpenAudioChannel() {
    if (mqtt_ == nullptr || !mqtt_->IsConnected()) {
        ESP_LOGI(TAG, "MQTT belum terhubung, mencoba menghubungkan sekarang");
        if (!StartMqttClient(true)) {
            return false;
        }
    }

    error_occurred_ = false;
    session_id_.clear();
    last_incoming_time_ = std::chrono::steady_clock::now();
    xEventGroupClearBits(event_group_handle_, MQTT_PROTOCOL_SERVER_HELLO_EVENT);

    if (!PublishText(GetHelloMessage(), true)) {
        return false;
    }

    EventBits_t bits = xEventGroupWaitBits(
        event_group_handle_,
        MQTT_PROTOCOL_SERVER_HELLO_EVENT,
        pdTRUE,
        pdFALSE,
        pdMS_TO_TICKS(kServerHelloTimeoutMs));
    if (!(bits & MQTT_PROTOCOL_SERVER_HELLO_EVENT)) {
        ESP_LOGE(TAG, "Waktu tunggu server hello habis");
        SetError(Lang::Strings::SERVER_TIMEOUT);
        return false;
    }

    std::lock_guard<std::mutex> lock(channel_mutex_);
    auto network = Board::GetInstance().GetNetwork();
    if (network == nullptr) {
        ESP_LOGE(TAG, "Antarmuka jaringan UDP tidak tersedia");
        SetError(Lang::Strings::SERVER_NOT_CONNECTED);
        return false;
    }
    udp_ = network->CreateUdp(2);
    if (udp_ == nullptr) {
        ESP_LOGE(TAG, "Gagal membuat soket UDP");
        SetError(Lang::Strings::SERVER_ERROR);
        return false;
    }
    udp_->OnMessage([this](const std::string& data) {
        /*
         * Format paket OPUS terenkripsi melalui UDP:
         * |type 1u|flags 1u|payload_len 2u|ssrc 4u|timestamp 4u|sequence 4u|
         * |payload payload_len|
         */
        if (aes_nonce_.size() != kAesNonceSize ||
            data.size() < kAesNonceSize) {
            ESP_LOGE(TAG, "Ukuran paket audio tidak valid: %u",
                static_cast<unsigned>(data.size()));
            return;
        }
        if (static_cast<uint8_t>(data[0]) != 0x01) {
            ESP_LOGE(TAG, "Tipe paket audio tidak valid: %02x",
                static_cast<unsigned>(static_cast<uint8_t>(data[0])));
            return;
        }

        uint16_t payload_size_be = 0;
        uint32_t timestamp_be = 0;
        uint32_t sequence_be = 0;
        memcpy(&payload_size_be, data.data() + 2, sizeof(payload_size_be));
        memcpy(&timestamp_be, data.data() + 8, sizeof(timestamp_be));
        memcpy(&sequence_be, data.data() + 12, sizeof(sequence_be));
        const size_t payload_size = ntohs(payload_size_be);
        if (data.size() != kAesNonceSize + payload_size) {
            ESP_LOGW(TAG, "Ukuran payload UDP tidak cocok: header=%u aktual=%u",
                static_cast<unsigned>(payload_size),
                static_cast<unsigned>(data.size() - kAesNonceSize));
            return;
        }

        const uint32_t timestamp = ntohl(timestamp_be);
        const uint32_t sequence = ntohl(sequence_be);
        if (remote_sequence_ != 0 && sequence <= remote_sequence_) {
            const int64_t now_us = esp_timer_get_time();
            if (now_us - last_sequence_warning_us_ >= kSequenceWarningIntervalUs) {
                last_sequence_warning_us_ = now_us;
                ESP_LOGW(TAG, "Paket audio lama: %lu, terakhir: %lu",
                    sequence, remote_sequence_);
            }
            return;
        }
        if (sequence != remote_sequence_ + 1) {
            const int64_t now_us = esp_timer_get_time();
            if (now_us - last_sequence_warning_us_ >= kSequenceWarningIntervalUs) {
                last_sequence_warning_us_ = now_us;
                ESP_LOGW(TAG, "Paket audio terlewat: diterima=%lu diharapkan=%lu",
                    sequence, remote_sequence_ + 1);
            }
        }

        const size_t decrypted_size = payload_size;
        size_t nc_off = 0;
        uint8_t stream_block[16] = {0};
        std::string nonce(data.data(), kAesNonceSize);
        auto packet = std::make_unique<AudioStreamPacket>();
        packet->sample_rate = server_sample_rate_;
        packet->frame_duration = server_frame_duration_;
        packet->timestamp = timestamp;
        packet->payload.resize(decrypted_size);

        int ret = mbedtls_aes_crypt_ctr(
            &aes_ctx_, decrypted_size, &nc_off,
            reinterpret_cast<unsigned char*>(nonce.data()), stream_block,
            reinterpret_cast<const unsigned char*>(data.data() + kAesNonceSize),
            reinterpret_cast<unsigned char*>(packet->payload.data()));
        if (ret != 0) {
            ESP_LOGE(TAG, "Gagal mendekripsi data audio, kode: %d", ret);
            return;
        }
        remote_sequence_ = sequence;

        if (on_incoming_audio_ != nullptr) {
            on_incoming_audio_(std::move(packet));
        }
        last_incoming_time_ = std::chrono::steady_clock::now();
    });

    if (!udp_->Connect(udp_server_, udp_port_)) {
        ESP_LOGE(TAG, "Gagal menghubungkan UDP ke %s:%d", udp_server_.c_str(), udp_port_);
        udp_.reset();
        SetError(Lang::Strings::SERVER_NOT_CONNECTED);
        return false;
    }

    last_incoming_time_ = std::chrono::steady_clock::now();
    if (on_audio_channel_opened_ != nullptr) {
        on_audio_channel_opened_();
    }
    return true;
}

std::string MqttProtocol::GetHelloMessage() {
    // Kirim pesan hello untuk meminta kanal audio UDP.
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
    std::lock_guard<std::mutex> lock(channel_mutex_);
    if (udp_ != nullptr) {
        ESP_LOGW(TAG, "Server hello tambahan diabaikan karena kanal audio sudah terbuka");
        return;
    }

    auto transport = cJSON_GetObjectItem(root, "transport");
    if (!cJSON_IsString(transport) || strcmp(transport->valuestring, "udp") != 0) {
        ESP_LOGE(TAG, "Transport server tidak valid");
        return;
    }

    auto session_id = cJSON_GetObjectItem(root, "session_id");
    if (!cJSON_IsString(session_id) || session_id->valuestring[0] == '\0') {
        ESP_LOGE(TAG, "Session ID server tidak valid");
        return;
    }
    session_id_ = session_id->valuestring;
    ESP_LOGI(TAG, "Session ID: %s", session_id_.c_str());

    // Ambil parameter audio dari pesan hello.
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
        ESP_LOGE(TAG, "Konfigurasi UDP tidak tersedia");
        return;
    }
    auto server = cJSON_GetObjectItem(udp, "server");
    auto port = cJSON_GetObjectItem(udp, "port");
    auto key = cJSON_GetObjectItem(udp, "key");
    auto nonce = cJSON_GetObjectItem(udp, "nonce");
    if (!cJSON_IsString(server) || !cJSON_IsNumber(port) ||
        !cJSON_IsString(key) || !cJSON_IsString(nonce) ||
        port->valueint <= 0 || port->valueint > 65535 ||
        strlen(key->valuestring) != 32 || strlen(nonce->valuestring) != 32) {
        ESP_LOGE(TAG, "Konfigurasi UDP atau AES dari server tidak valid");
        return;
    }

    udp_server_ = server->valuestring;
    udp_port_ = port->valueint;
    aes_nonce_ = DecodeHexString(nonce->valuestring);
    std::string aes_key = DecodeHexString(key->valuestring);
    if (aes_nonce_.size() != 16 || aes_key.size() != 16 ||
        mbedtls_aes_setkey_enc(&aes_ctx_,
            reinterpret_cast<const unsigned char*>(aes_key.data()), 128) != 0) {
        ESP_LOGE(TAG, "Gagal menyiapkan kunci AES kanal audio");
        return;
    }
    local_sequence_ = 0;
    remote_sequence_ = 0;
    last_sequence_warning_us_ = 0;
    xEventGroupSetBits(event_group_handle_, MQTT_PROTOCOL_SERVER_HELLO_EVENT);
}

// Ubah satu karakter heksadesimal menjadi nilai binernya.
static inline uint8_t CharToHex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

std::string MqttProtocol::DecodeHexString(const std::string& hex_string) {
    std::string decoded;
    if ((hex_string.size() & 1U) != 0U) {
        return decoded;
    }
    decoded.reserve(hex_string.size() / 2);
    for (size_t i = 0; i < hex_string.size(); i += 2) {
        if (!std::isxdigit(static_cast<unsigned char>(hex_string[i])) ||
            !std::isxdigit(static_cast<unsigned char>(hex_string[i + 1]))) {
            decoded.clear();
            return decoded;
        }
        char byte = (CharToHex(hex_string[i]) << 4) | CharToHex(hex_string[i + 1]);
        decoded.push_back(byte);
    }
    return decoded;
}

bool MqttProtocol::IsAudioChannelOpened() const {
    return udp_ != nullptr && !error_occurred_ && !IsTimeout();
}
