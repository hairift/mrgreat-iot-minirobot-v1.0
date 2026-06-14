#include "application.h"
#include "board.h"
#include "display.h"
#include "system_info.h"
#include "audio_codec.h"
#include "mqtt_protocol.h"
#include "websocket_protocol.h"
#include "assets/lang_config.h"
#include "mcp_server.h"
#include "assets.h"
#include "settings.h"
#include "servo_controller.h"

#include <cstring>
#include <algorithm>
#include <esp_log.h>
#include <cJSON.h>
#include <driver/gpio.h>
#include <arpa/inet.h>
#include <font_awesome.h>

#define TAG "Application"

namespace {

const char* GetJsonString(const cJSON* root, const char* key) {
    auto item = cJSON_GetObjectItem(root, key);
    return cJSON_IsString(item) ? item->valuestring : nullptr;
}

constexpr int kSpeakingIdleRecoverySeconds = 25;
constexpr int kListeningIdleCloseSeconds = 120;
constexpr int64_t kTtsDrainGraceUs = 1200 * 1000;
constexpr int64_t kProtocolRecoveryIntervalUs = 5 * 1000 * 1000;

void LogMessagePreview(const char* prefix, const char* text) {
    if (text == nullptr) {
        return;
    }

    constexpr size_t kPreviewLength = 240;
    size_t text_length = strlen(text);
    size_t preview_length = std::min(text_length, kPreviewLength);
    ESP_LOGI(TAG, "%s %.*s%s", prefix, static_cast<int>(preview_length), text,
        text_length > preview_length ? "..." : "");
}

#if CONFIG_RECEIVE_CUSTOM_MESSAGE
std::string JsonToString(const cJSON* root) {
    char* json_str = cJSON_PrintUnformatted(root);
    std::string result = json_str ? json_str : "{}";
    if (json_str != nullptr) {
        cJSON_free(json_str);
    }
    return result;
}
#endif

}  // ruang nama lokal


Application::Application() {
    event_group_ = xEventGroupCreate();

#if CONFIG_USE_DEVICE_AEC && CONFIG_USE_SERVER_AEC
#error "CONFIG_USE_DEVICE_AEC and CONFIG_USE_SERVER_AEC cannot be enabled at the same time"
#elif CONFIG_USE_DEVICE_AEC
    aec_mode_ = kAecOnDeviceSide;
#elif CONFIG_USE_SERVER_AEC
    aec_mode_ = kAecOnServerSide;
#else
    aec_mode_ = kAecOff;
#endif

    esp_timer_create_args_t clock_timer_args = {
        .callback = [](void* arg) {
            Application* app = (Application*)arg;
            xEventGroupSetBits(app->event_group_, MAIN_EVENT_CLOCK_TICK);
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "clock_timer",
        .skip_unhandled_events = true
    };
    esp_timer_create(&clock_timer_args, &clock_timer_handle_);
}

Application::~Application() {
    if (clock_timer_handle_ != nullptr) {
        esp_timer_stop(clock_timer_handle_);
        esp_timer_delete(clock_timer_handle_);
    }
    vEventGroupDelete(event_group_);
}

bool Application::SetDeviceState(DeviceState state) {
    return state_machine_.TransitionTo(state);
}

void Application::Initialize() {
    auto& board = Board::GetInstance();
    SetDeviceState(kDeviceStateStarting);

    // Atur layar
    auto display = board.GetDisplay();
    display->SetupUI();
    // Cetak info nama dan versi papan
    display->SetChatMessage("system", SystemInfo::GetUserAgent().c_str());

    // Atur layanan audio
    auto codec = board.GetAudioCodec();
    audio_service_.Initialize(codec);
    audio_service_.Start();

    AudioServiceCallbacks callbacks;
    callbacks.on_send_queue_available = [this]() {
        xEventGroupSetBits(event_group_, MAIN_EVENT_SEND_AUDIO);
    };
    callbacks.on_wake_word_detected = [this](const std::string& wake_word) {
        xEventGroupSetBits(event_group_, MAIN_EVENT_WAKE_WORD_DETECTED);
    };
    callbacks.on_vad_change = [this](bool speaking) {
        xEventGroupSetBits(event_group_, MAIN_EVENT_VAD_CHANGE);
    };
    audio_service_.SetCallbacks(callbacks);

    // Tambahkan pendengar perubahan status
    state_machine_.AddStateChangeListener([this](DeviceState old_state, DeviceState new_state) {
        xEventGroupSetBits(event_group_, MAIN_EVENT_STATE_CHANGED);
    });

    // Mulai pewaktu jam untuk memperbarui bilah status
    esp_timer_start_periodic(clock_timer_handle_, 1000000);

    // Tambahkan tool MCP umum (hanya sekali selama inisialisasi)
    auto& mcp_server = McpServer::GetInstance();
    mcp_server.AddCommonTools();
    mcp_server.AddUserOnlyTools();

    // Atur fungsi panggil balik kejadian jaringan untuk pembaruan antarmuka dan penanganan status jaringan
    board.SetNetworkEventCallback([this](NetworkEvent event, const std::string& data) {
        auto display = Board::GetInstance().GetDisplay();
        
        switch (event) {
            case NetworkEvent::Scanning:
                display->ShowNotification(Lang::Strings::SCANNING_WIFI, 30000);
                xEventGroupSetBits(event_group_, MAIN_EVENT_NETWORK_DISCONNECTED);
                break;
            case NetworkEvent::Connecting: {
                if (data.empty()) {
                    // Jaringan seluler, mendaftar tanpa info operator
                    display->SetStatus(Lang::Strings::REGISTERING_NETWORK);
                } else {
                    // Wi-Fi atau seluler dengan info operator
                    std::string msg = Lang::Strings::CONNECT_TO;
                    msg += data;
                    msg += "...";
                    display->ShowNotification(msg.c_str(), 30000);
                }
                break;
            }
            case NetworkEvent::Connected: {
                std::string msg = Lang::Strings::CONNECTED_TO;
                msg += data;
                display->ShowNotification(msg.c_str(), 30000);
                xEventGroupSetBits(event_group_, MAIN_EVENT_NETWORK_CONNECTED);
                break;
            }
            case NetworkEvent::Disconnected:
                xEventGroupSetBits(event_group_, MAIN_EVENT_NETWORK_DISCONNECTED);
                break;
            case NetworkEvent::WifiConfigModeEnter:
                // Mode konfigurasi Wi-Fi masuk ditangani oleh WifiBoard secara internal
                break;
            case NetworkEvent::WifiConfigModeExit:
                // Mode konfigurasi Wi-Fi keluar ditangani oleh WifiBoard secara internal
                break;
            // Kejadian khusus modem seluler
            case NetworkEvent::ModemDetecting:
                display->SetStatus(Lang::Strings::DETECTING_MODULE);
                break;
            case NetworkEvent::ModemErrorNoSim:
                Alert(Lang::Strings::ERROR, Lang::Strings::PIN_ERROR, "triangle_exclamation", Lang::Sounds::OGG_ERR_PIN);
                break;
            case NetworkEvent::ModemErrorRegDenied:
                Alert(Lang::Strings::ERROR, Lang::Strings::REG_ERROR, "triangle_exclamation", Lang::Sounds::OGG_ERR_REG);
                break;
            case NetworkEvent::ModemErrorInitFailed:
                Alert(Lang::Strings::ERROR, Lang::Strings::MODEM_INIT_ERROR, "triangle_exclamation", Lang::Sounds::OGG_EXCLAMATION);
                break;
            case NetworkEvent::ModemErrorTimeout:
                display->SetStatus(Lang::Strings::REGISTERING_NETWORK);
                break;
        }
    });

    // Mulai jaringan secara asinkron
    board.StartNetwork();

    // Perbarui bilah status segera untuk menunjukkan status jaringan
    display->UpdateStatusBar(true);
}

void Application::Run() {
    // Atur prioritas tugas utama ke 10
    vTaskPrioritySet(nullptr, 10);

    const EventBits_t ALL_EVENTS = 
        MAIN_EVENT_SCHEDULE |
        MAIN_EVENT_SEND_AUDIO |
        MAIN_EVENT_WAKE_WORD_DETECTED |
        MAIN_EVENT_VAD_CHANGE |
        MAIN_EVENT_CLOCK_TICK |
        MAIN_EVENT_ERROR |
        MAIN_EVENT_NETWORK_CONNECTED |
        MAIN_EVENT_NETWORK_DISCONNECTED |
        MAIN_EVENT_TOGGLE_CHAT |
        MAIN_EVENT_START_LISTENING |
        MAIN_EVENT_STOP_LISTENING |
        MAIN_EVENT_ACTIVATION_DONE |
        MAIN_EVENT_STATE_CHANGED;

    while (true) {
        auto bits = xEventGroupWaitBits(event_group_, ALL_EVENTS, pdTRUE, pdFALSE, portMAX_DELAY);

        if (bits & MAIN_EVENT_ERROR) {
            SetDeviceState(kDeviceStateIdle);
            Alert(Lang::Strings::ERROR, last_error_message_.c_str(), "circle_xmark", Lang::Sounds::OGG_EXCLAMATION);
        }

        if (bits & MAIN_EVENT_NETWORK_CONNECTED) {
            HandleNetworkConnectedEvent();
        }

        if (bits & MAIN_EVENT_NETWORK_DISCONNECTED) {
            HandleNetworkDisconnectedEvent();
        }

        if (bits & MAIN_EVENT_ACTIVATION_DONE) {
            HandleActivationDoneEvent();
        }

        if (bits & MAIN_EVENT_STATE_CHANGED) {
            HandleStateChangedEvent();
        }

        if (bits & MAIN_EVENT_TOGGLE_CHAT) {
            HandleToggleChatEvent();
        }

        if (bits & MAIN_EVENT_START_LISTENING) {
            HandleStartListeningEvent();
        }

        if (bits & MAIN_EVENT_STOP_LISTENING) {
            HandleStopListeningEvent();
        }

        if (bits & MAIN_EVENT_SEND_AUDIO) {
            while (auto packet = audio_service_.PopPacketFromSendQueue()) {
                if (protocol_ && !protocol_->SendAudio(std::move(packet))) {
                    break;
                }
            }
        }

        if (bits & MAIN_EVENT_WAKE_WORD_DETECTED) {
            HandleWakeWordDetectedEvent();
        }

        if (bits & MAIN_EVENT_VAD_CHANGE) {
            if (GetDeviceState() == kDeviceStateListening) {
                auto led = Board::GetInstance().GetLed();
                led->OnStateChanged();
            }
        }

        if (bits & MAIN_EVENT_SCHEDULE) {
            std::unique_lock<std::mutex> lock(mutex_);
            auto tasks = std::move(main_tasks_);
            lock.unlock();
            for (auto& task : tasks) {
                task();
            }
        }

        if (bits & MAIN_EVENT_CLOCK_TICK) {
            clock_ticks_++;
            auto display = Board::GetInstance().GetDisplay();
            display->UpdateStatusBar();

            auto state = GetDeviceState();
            if (state == kDeviceStateSpeaking) {
                int64_t now_us = esp_timer_get_time();
                int64_t last_audio_us = last_tts_audio_us_.load();
                if (tts_stop_pending_.load() && audio_service_.IsIdle() &&
                    last_audio_us > 0 && (now_us - last_audio_us) >= kTtsDrainGraceUs) {
                    ESP_LOGI(TAG, "Seluruh audio TTS selesai diputar");
                    tts_stream_active_ = false;
                    tts_stop_pending_ = false;
                    ServoController::GetInstance().SetTtsStreamActive(false);
                    if (listening_mode_ == kListeningModeManualStop) {
                        SetDeviceState(kDeviceStateIdle);
                    } else {
                        SetDeviceState(kDeviceStateListening);
                    }
                }

                if (audio_service_.IsIdle()) {
                    ++speaking_idle_ticks_;
                } else {
                    speaking_idle_ticks_ = 0;
                }

                if (speaking_idle_ticks_ >= kSpeakingIdleRecoverySeconds) {
                    ESP_LOGW(TAG, "State speaking idle terlalu lama, pulihkan percakapan");
                    speaking_idle_ticks_ = 0;
                    if (protocol_ && protocol_->IsAudioChannelOpened()) {
                        protocol_->SendAbortSpeaking(kAbortReasonNone);
                    }
                    if (listening_mode_ == kListeningModeManualStop) {
                        SetDeviceState(kDeviceStateIdle);
                    } else {
                        SetDeviceState(kDeviceStateListening);
                    }
                }
            } else {
                speaking_idle_ticks_ = 0;
            }

            if (protocol_reopen_pending_.load() &&
                esp_timer_get_time() >= next_protocol_recovery_us_.load()) {
                TryRecoverProtocol();
            }

            if (state == kDeviceStateListening &&
                clock_ticks_ >= kListeningIdleCloseSeconds &&
                !audio_service_.IsVoiceDetected()) {
                ESP_LOGW(TAG, "State listening idle terlalu lama, tutup kanal audio agar tidak macet");
                if (protocol_ && protocol_->IsAudioChannelOpened()) {
                    protocol_->CloseAudioChannel();
                } else {
                    SetDeviceState(kDeviceStateIdle);
                }
            }
        
            // Cetak info diagnostik secukupnya dan hindari membebani UART saat audio sedang diputar.
            if (clock_ticks_ % 30 == 0 && GetDeviceState() != kDeviceStateSpeaking) {
                SystemInfo::PrintHeapStats();
            }
        }
    }
}

void Application::HandleNetworkConnectedEvent() {
    ESP_LOGI(TAG, "Network connected");
    network_connected_ = true;
    auto state = GetDeviceState();

    if (state == kDeviceStateStarting || state == kDeviceStateWifiConfiguring) {
        // Jaringan siap, mulai aktivasi
        SetDeviceState(kDeviceStateActivating);
        if (activation_task_handle_ != nullptr) {
            ESP_LOGW(TAG, "Activation task already running");
            return;
        }

        xTaskCreate([](void* arg) {
            Application* app = static_cast<Application*>(arg);
            app->ActivationTask();
            app->activation_task_handle_ = nullptr;
            vTaskDelete(NULL);
        }, "activation", 4096 * 2, this, 2, &activation_task_handle_);
    }

    if (protocol_reopen_pending_.load()) {
        next_protocol_recovery_us_ = esp_timer_get_time();
        TryRecoverProtocol();
    }

    // Perbarui bilah status segera untuk menampilkan status jaringan
    auto display = Board::GetInstance().GetDisplay();
    display->UpdateStatusBar(true);
}

void Application::HandleNetworkDisconnectedEvent() {
    network_connected_ = false;
    tts_stream_active_ = false;
    tts_stop_pending_ = false;
    last_tts_audio_us_ = 0;
    ServoController::GetInstance().SetTtsStreamActive(false);

    // Tutup percakapan saat ini saat jaringan terputus
    auto state = GetDeviceState();
    if (protocol_ && (state == kDeviceStateConnecting || state == kDeviceStateListening || state == kDeviceStateSpeaking)) {
        MarkProtocolForRecovery();
        ESP_LOGI(TAG, "Closing audio channel due to network disconnection");
        protocol_->CloseAudioChannel(false);
    }

    // Perbarui bilah status segera untuk menampilkan status jaringan
    auto display = Board::GetInstance().GetDisplay();
    display->UpdateStatusBar(true);
}

void Application::HandleActivationDoneEvent() {
    ESP_LOGI(TAG, "Activation done");

    SystemInfo::PrintHeapStats();
    SetDeviceState(kDeviceStateIdle);

    has_server_time_ = ota_->HasServerTime();

    auto display = Board::GetInstance().GetDisplay();
    std::string message = std::string(Lang::Strings::VERSION) + ota_->GetCurrentVersion();
    display->ShowNotification(message.c_str());
    display->SetChatMessage("system", "");

    // Lepaskan objek OTA setelah aktivasi selesai
    ota_.reset();
    auto& board = Board::GetInstance();
    board.SetPowerSaveLevel(PowerSaveLevel::BALANCED);

    Schedule([this]() {
        // Mainkan suara sukses untuk menunjukkan perangkat siap
        audio_service_.PlaySound(Lang::Sounds::OGG_SUCCESS);
    });
}

void Application::ActivationTask() {
    // Buat objek OTA untuk proses aktivasi
    ota_ = std::make_unique<Ota>();

    // Periksa versi aset baru
    CheckAssetsVersion();

    // Periksa versi firmware baru
    CheckNewVersion();

    // Inisialisasi protokol
    InitializeProtocol();

    // Kirim sinyal selesai ke loop utama
    xEventGroupSetBits(event_group_, MAIN_EVENT_ACTIVATION_DONE);
}

void Application::CheckAssetsVersion() {
    // Hanya izinkan fungsi pemeriksaan versi aset dipanggil sekali
    if (assets_version_checked_) {
        return;
    }
    assets_version_checked_ = true;

    auto& board = Board::GetInstance();
    auto display = board.GetDisplay();
    auto& assets = Assets::GetInstance();

    if (!assets.partition_valid()) {
        ESP_LOGW(TAG, "Assets partition is disabled for board %s", BOARD_NAME);
        return;
    }
    
    Settings settings("assets", true);
    // Periksa apakah ada aset baru yang perlu diunduh
    std::string download_url = settings.GetString("download_url");

    if (!download_url.empty()) {
        settings.EraseKey("download_url");

        char message[256];
        snprintf(message, sizeof(message), Lang::Strings::FOUND_NEW_ASSETS, download_url.c_str());
        Alert(Lang::Strings::LOADING_ASSETS, message, "cloud_arrow_down", Lang::Sounds::OGG_UPGRADE);
        
        // Tunggu layanan audio idle selama 3 detik
        vTaskDelay(pdMS_TO_TICKS(3000));
        SetDeviceState(kDeviceStateUpgrading);
        board.SetPowerSaveLevel(PowerSaveLevel::PERFORMANCE);
        display->SetChatMessage("system", Lang::Strings::PLEASE_WAIT);

        bool success = assets.Download(download_url, [this, display](int progress, size_t speed) -> void {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%d%% %uKB/s", progress, speed / 1024);
            Schedule([display, message = std::string(buffer)]() {
                display->SetChatMessage("system", message.c_str());
            });
        });

        board.SetPowerSaveLevel(PowerSaveLevel::BALANCED);
        vTaskDelay(pdMS_TO_TICKS(1000));

        if (!success) {
            Alert(Lang::Strings::ERROR, Lang::Strings::DOWNLOAD_ASSETS_FAILED, "circle_xmark", Lang::Sounds::OGG_EXCLAMATION);
            vTaskDelay(pdMS_TO_TICKS(2000));
            SetDeviceState(kDeviceStateActivating);
            return;
        }
    }

        // Terapkan aset
    assets.Apply();
    display->SetChatMessage("system", "");
    display->SetEmotion("microchip_ai");
}

void Application::CheckNewVersion() {
    const int MAX_RETRY = 10;
    int retry_count = 0;
    int retry_delay = 10; // Jeda awal untuk mencoba lagi dalam satuan detik

    auto& board = Board::GetInstance();
    while (true) {
        auto display = board.GetDisplay();
        display->SetStatus(Lang::Strings::CHECKING_NEW_VERSION);

        esp_err_t err = ota_->CheckVersion();
        if (err != ESP_OK) {
            retry_count++;
            if (retry_count >= MAX_RETRY) {
                ESP_LOGE(TAG, "Too many retries, exit version check");
                return;
            }

            char error_message[128];
            snprintf(error_message, sizeof(error_message), "code=%d, url=%s", err, ota_->GetCheckVersionUrl().c_str());
            char buffer[256];
            snprintf(buffer, sizeof(buffer), Lang::Strings::CHECK_NEW_VERSION_FAILED, retry_delay, error_message);
            Alert(Lang::Strings::ERROR, buffer, "cloud_slash", Lang::Sounds::OGG_EXCLAMATION);

            ESP_LOGW(TAG, "Check new version failed, retry in %d seconds (%d/%d)", retry_delay, retry_count, MAX_RETRY);
            for (int i = 0; i < retry_delay; i++) {
                vTaskDelay(pdMS_TO_TICKS(1000));
                if (GetDeviceState() == kDeviceStateIdle) {
                    break;
                }
            }
            retry_delay *= 2; // Gandakan jeda percobaan ulang
            continue;
        }
        retry_count = 0;
        retry_delay = 10; // Kembalikan jeda percobaan ulang ke nilai awal

        if (ota_->HasNewVersion()) {
            if (UpgradeFirmware(ota_->GetFirmwareUrl(), ota_->GetFirmwareVersion())) {
                return; // Baris ini tidak akan pernah tercapai setelah mulai ulang
            }
            // Jika upgrade gagal, lanjutkan operasi normal
        }

        // Jika tidak ada versi baru, tandai versi saat ini sebagai valid
        ota_->MarkCurrentVersionValid();
        if (!ota_->HasActivationCode() && !ota_->HasActivationChallenge()) {
            // Keluar dari loop jika pemeriksaan versi baru sudah selesai
            break;
        }

        display->SetStatus(Lang::Strings::ACTIVATION);
        // Tampilkan kode aktivasi ke pengguna lalu tunggu masukan dari pengguna
        if (ota_->HasActivationCode()) {
            ShowActivationCode(ota_->GetActivationCode(), ota_->GetActivationMessage());
        }

        // Bagian ini akan menahan loop sampai aktivasi selesai atau timeout
        for (int i = 0; i < 10; ++i) {
            ESP_LOGI(TAG, "Activating... %d/%d", i + 1, 10);
            esp_err_t err = ota_->Activate();
            if (err == ESP_OK) {
                break;
            } else if (err == ESP_ERR_TIMEOUT) {
                vTaskDelay(pdMS_TO_TICKS(3000));
            } else {
                vTaskDelay(pdMS_TO_TICKS(10000));
            }
            if (GetDeviceState() == kDeviceStateIdle) {
                break;
            }
        }
    }
}

void Application::InitializeProtocol() {
    auto& board = Board::GetInstance();
    auto display = board.GetDisplay();
    auto codec = board.GetAudioCodec();

    display->SetStatus(Lang::Strings::LOADING_PROTOCOL);

    if (ota_->HasMqttConfig()) {
        protocol_ = std::make_unique<MqttProtocol>();
    } else if (ota_->HasWebsocketConfig()) {
        protocol_ = std::make_unique<WebsocketProtocol>();
    } else {
        ESP_LOGW(TAG, "No protocol specified in the OTA config, using MQTT");
        protocol_ = std::make_unique<MqttProtocol>();
    }

    protocol_->OnConnected([this]() {
        Schedule([this]() {
            DismissAlert();
            if (protocol_reopen_pending_.load()) {
                next_protocol_recovery_us_ = esp_timer_get_time();
                TryRecoverProtocol();
            }
        });
    });

    protocol_->OnDisconnected([this]() {
        MarkProtocolForRecovery();
        Schedule([this]() {
            if (!protocol_) {
                return;
            }

            auto state = GetDeviceState();
            if (state == kDeviceStateConnecting ||
                state == kDeviceStateListening ||
                state == kDeviceStateSpeaking) {
                ESP_LOGW(TAG, "Koneksi protokol terputus saat sesi aktif, menutup kanal audio");
                protocol_->CloseAudioChannel(false);
            }
        });
    });

    protocol_->OnNetworkError([this](const std::string& message) {
        last_error_message_ = message;
        xEventGroupSetBits(event_group_, MAIN_EVENT_ERROR);
    });
    
    protocol_->OnIncomingAudio([this](std::unique_ptr<AudioStreamPacket> packet) {
        if (tts_stream_active_.load() || GetDeviceState() == kDeviceStateSpeaking) {
            last_tts_audio_us_ = esp_timer_get_time();
            if (!audio_service_.PushPacketToDecodeQueue(std::move(packet))) {
                uint32_t dropped = tts_packets_dropped_.fetch_add(1) + 1;
                if (dropped == 1 || dropped % 10 == 0) {
                    ESP_LOGW(TAG, "Antrian decode audio penuh, paket TTS dilewati (%lu total)",
                        static_cast<unsigned long>(dropped));
                }
            }
        }
    });
    
    protocol_->OnAudioChannelOpened([this, codec, &board]() {
        board.SetPowerSaveLevel(PowerSaveLevel::PERFORMANCE);
        if (protocol_->server_sample_rate() != codec->output_sample_rate()) {
            ESP_LOGW(TAG, "Server sample rate %d does not match device output sample rate %d, resampling may cause distortion",
                protocol_->server_sample_rate(), codec->output_sample_rate());
        }
    });
    
    protocol_->OnAudioChannelClosed([this, &board]() {
        board.SetPowerSaveLevel(PowerSaveLevel::BALANCED);
        Schedule([this]() {
            auto display = Board::GetInstance().GetDisplay();
            display->SetChatMessage("system", "");
            if (protocol_reopen_pending_.load() && network_connected_.load()) {
                SetDeviceState(kDeviceStateConnecting);
            } else {
                SetDeviceState(kDeviceStateIdle);
            }
        });
    });
    
    protocol_->OnIncomingJson([this, display](const cJSON* root) {
        const char* type = GetJsonString(root, "type");
        if (type == nullptr) {
            ESP_LOGW(TAG, "Incoming JSON missing valid type");
            return;
        }
        if (strcmp(type, "tts") == 0) {
            const char* state = GetJsonString(root, "state");
            if (state == nullptr) {
                ESP_LOGW(TAG, "TTS message missing valid state");
                return;
            }
            if (strcmp(state, "start") == 0) {
                audio_service_.ResetDecoder();
                last_tts_audio_us_ = esp_timer_get_time();
                tts_stop_pending_ = false;
                tts_stream_active_ = true;
                tts_packets_dropped_ = 0;
                ServoController::GetInstance().SetTtsStreamActive(true);
                ServoController::GetInstance().SetSpeaking(true);
                Schedule([this]() {
                    aborted_ = false;
                    SetDeviceState(kDeviceStateSpeaking);
                });
            } else if (strcmp(state, "stop") == 0) {
                tts_stop_pending_ = true;
                ServoController::GetInstance().SetTtsStreamActive(false);
                ServoController::GetInstance().SetSpeaking(false);
                if (last_tts_audio_us_.load() == 0) {
                    last_tts_audio_us_ = esp_timer_get_time();
                }
            } else if (strcmp(state, "sentence_start") == 0) {
                const char* text = GetJsonString(root, "text");
                if (text != nullptr) {
                    LogMessagePreview("<<", text);
                    ServoController::GetInstance().NotifyTtsSentence();
                    Schedule([display, message = std::string(text)]() {
                        display->SetChatMessage("assistant", message.c_str());
                    });
                }
            }
        } else if (strcmp(type, "stt") == 0) {
            const char* text = GetJsonString(root, "text");
            if (text != nullptr) {
                LogMessagePreview(">>", text);
                // Setiap pertanyaan baru kembali ke gestur umum sampai tool pencarian benar-benar dipakai.
                ServoController::GetInstance().SetKnowledgeSearchActive(false);
                ServoMove servo_cmd = ServoController::DetectCommand(text);
                bool handled_servo_command = false;
                if (servo_cmd == ServoMove::ENABLE_SERVO) {
                    ServoController::GetInstance().Enable();
                    handled_servo_command = true;
                } else if (servo_cmd == ServoMove::DISABLE_SERVO) {
                    ServoController::GetInstance().Disable();
                    handled_servo_command = true;
                } else if (servo_cmd != ServoMove::NONE) {
                    // Gerakan tunggal waktu nyata dieksekusi langsung
                    ServoController::GetInstance().ExecuteMove(servo_cmd);
                    handled_servo_command = true;
                }
                last_stt_had_servo_command_ = handled_servo_command;
                Schedule([display, message = std::string(text)]() {
                    display->SetChatMessage("user", message.c_str());
                });
            }
        } else if (strcmp(type, "llm") == 0) {
            const char* emotion = GetJsonString(root, "emotion");
            if (emotion != nullptr) {
                if (last_stt_had_servo_command_.load()) {
                    ESP_LOGI(TAG, "Lewati gestur emosi '%s' karena turn ini berisi perintah servo", emotion);
                } else {
                    ServoController::GetInstance().SetEmotion(emotion);
                    Schedule([display, emotion_str = std::string(emotion)]() {
                        display->SetEmotion(emotion_str.c_str());
                    });
                }
            }
        } else if (strcmp(type, "mcp") == 0) {
            auto payload = cJSON_GetObjectItem(root, "payload");
            if (cJSON_IsObject(payload)) {
                McpServer::GetInstance().ParseMessage(payload);
            } else {
                ESP_LOGW(TAG, "MCP payload missing or invalid");
            }
        } else if (strcmp(type, "system") == 0) {
            const char* command = GetJsonString(root, "command");
            if (command != nullptr) {
                ESP_LOGI(TAG, "System command: %s", command);
                if (strcmp(command, "reboot") == 0) {
                    Schedule([this]() {
                        Reboot();
                    });
                } else {
                    ESP_LOGW(TAG, "Unknown system command: %s", command);
                }
            }
        } else if (strcmp(type, "alert") == 0) {
            const char* status = GetJsonString(root, "status");
            const char* message = GetJsonString(root, "message");
            const char* emotion = GetJsonString(root, "emotion");
            if (status != nullptr && message != nullptr && emotion != nullptr) {
                Alert(status, message, emotion, Lang::Sounds::OGG_VIBRATION);
            } else {
                ESP_LOGW(TAG, "Alert command requires status, message and emotion");
            }
#if CONFIG_RECEIVE_CUSTOM_MESSAGE
        } else if (strcmp(type, "custom") == 0) {
            auto payload = cJSON_GetObjectItem(root, "payload");
            ESP_LOGI(TAG, "Received custom message: %s", JsonToString(root).c_str());
            if (cJSON_IsObject(payload)) {
                Schedule([display, payload_str = JsonToString(payload)]() {
                    display->SetChatMessage("system", payload_str.c_str());
                });
            } else {
                ESP_LOGW(TAG, "Invalid custom message format: missing payload");
            }
#endif
        } else {
            ESP_LOGW(TAG, "Unknown message type: %s", type);
        }
    });
    
    protocol_->Start();
}

void Application::ShowActivationCode(const std::string& code, const std::string& message) {
    struct digit_sound {
        char digit;
        const std::string_view& sound;
    };
    static const std::array<digit_sound, 10> digit_sounds{{
        digit_sound{'0', Lang::Sounds::OGG_0},
        digit_sound{'1', Lang::Sounds::OGG_1}, 
        digit_sound{'2', Lang::Sounds::OGG_2},
        digit_sound{'3', Lang::Sounds::OGG_3},
        digit_sound{'4', Lang::Sounds::OGG_4},
        digit_sound{'5', Lang::Sounds::OGG_5},
        digit_sound{'6', Lang::Sounds::OGG_6},
        digit_sound{'7', Lang::Sounds::OGG_7},
        digit_sound{'8', Lang::Sounds::OGG_8},
        digit_sound{'9', Lang::Sounds::OGG_9}
    }};

    // Bagian ini memakai sekitar 9KB SRAM, jadi perlu menunggu sampai selesai
    Alert(Lang::Strings::ACTIVATION, message.c_str(), "link", Lang::Sounds::OGG_ACTIVATION);

    for (const auto& digit : code) {
        auto it = std::find_if(digit_sounds.begin(), digit_sounds.end(),
            [digit](const digit_sound& ds) { return ds.digit == digit; });
        if (it != digit_sounds.end()) {
            audio_service_.PlaySound(it->sound);
        }
    }
}

void Application::Alert(const char* status, const char* message, const char* emotion, const std::string_view& sound) {
    ESP_LOGW(TAG, "Alert [%s] %s: %s", emotion, status, message);
    auto display = Board::GetInstance().GetDisplay();
    display->SetStatus(status);
    display->SetEmotion(emotion);
    display->SetChatMessage("system", message);
    if (!sound.empty()) {
        audio_service_.PlaySound(sound);
    }
}

void Application::DismissAlert() {
    if (GetDeviceState() == kDeviceStateIdle) {
        auto display = Board::GetInstance().GetDisplay();
        display->SetStatus(Lang::Strings::STANDBY);
        display->SetEmotion("neutral");
        display->SetChatMessage("system", "");
    }
}

void Application::ToggleChatState() {
    xEventGroupSetBits(event_group_, MAIN_EVENT_TOGGLE_CHAT);
}

void Application::StartListening() {
    xEventGroupSetBits(event_group_, MAIN_EVENT_START_LISTENING);
}

void Application::StopListening() {
    xEventGroupSetBits(event_group_, MAIN_EVENT_STOP_LISTENING);
}

void Application::HandleToggleChatEvent() {
    auto state = GetDeviceState();
    
    if (state == kDeviceStateActivating) {
        SetDeviceState(kDeviceStateIdle);
        return;
    } else if (state == kDeviceStateWifiConfiguring) {
        audio_service_.EnableAudioTesting(true);
        SetDeviceState(kDeviceStateAudioTesting);
        return;
    } else if (state == kDeviceStateAudioTesting) {
        audio_service_.EnableAudioTesting(false);
        SetDeviceState(kDeviceStateWifiConfiguring);
        return;
    }

    if (!protocol_) {
        ESP_LOGE(TAG, "Protocol not initialized");
        return;
    }

    if (state == kDeviceStateIdle) {
        ListeningMode mode = GetDefaultListeningMode();
        if (!protocol_->IsAudioChannelOpened()) {
            SetDeviceState(kDeviceStateConnecting);
            // Jadwalkan dulu agar perubahan status diproses lebih dulu untuk pembaruan antarmuka
            Schedule([this, mode]() {
                ContinueOpenAudioChannel(mode);
            });
            return;
        }
        SetListeningMode(mode);
    } else if (state == kDeviceStateSpeaking) {
        AbortSpeaking(kAbortReasonNone);
    } else if (state == kDeviceStateListening) {
        protocol_->CloseAudioChannel();
    }
}

void Application::ContinueOpenAudioChannel(ListeningMode mode) {
    // Periksa status sekali lagi jika berubah saat proses penjadwalan
    if (GetDeviceState() != kDeviceStateConnecting) {
        return;
    }

    if (!protocol_->IsAudioChannelOpened()) {
        if (!protocol_->OpenAudioChannel()) {
            SetDeviceState(kDeviceStateIdle);
            return;
        }
    }

    SetListeningMode(mode);
}

void Application::OpenAudioChannelAsync(ListeningMode mode, const std::string& wake_word) {
    if (audio_channel_opening_.exchange(true)) {
        ESP_LOGW(TAG, "Pembukaan kanal audio masih berjalan, permintaan baru diabaikan");
        return;
    }

    struct OpenRequest {
        Application* app;
        ListeningMode mode;
        std::string wake_word;
    };

    auto request = new OpenRequest{this, mode, wake_word};
    BaseType_t created = xTaskCreate([](void* arg) {
        auto request = static_cast<OpenRequest*>(arg);
        Application* app = request->app;
        ListeningMode mode = request->mode;
        std::string wake_word = std::move(request->wake_word);
        delete request;

        app->Schedule([app, mode, wake_word = std::move(wake_word)]() {
            app->audio_channel_opening_ = false;
            if (!wake_word.empty()) {
                app->ContinueWakeWordInvoke(wake_word);
            } else {
                app->ContinueOpenAudioChannel(mode);
            }
        });
        vTaskDelete(nullptr);
    }, "audio_channel_open", 4096, request, 3, nullptr);

    if (created != pdPASS) {
        delete request;
        audio_channel_opening_ = false;
        SetDeviceState(kDeviceStateIdle);
        ESP_LOGE(TAG, "Gagal membuat tugas pembukaan kanal audio");
    }
}

void Application::MarkProtocolForRecovery() {
    auto state = GetDeviceState();
    if (state != kDeviceStateConnecting &&
        state != kDeviceStateListening &&
        state != kDeviceStateSpeaking) {
        return;
    }
    recovery_listening_mode_ = static_cast<int>(listening_mode_);
    protocol_reopen_pending_ = true;
    next_protocol_recovery_us_ = esp_timer_get_time() + kProtocolRecoveryIntervalUs;
}

void Application::TryRecoverProtocol() {
    if (!protocol_reopen_pending_.load() || !network_connected_.load() ||
        protocol_ == nullptr || audio_channel_opening_.load()) {
        return;
    }
    next_protocol_recovery_us_ = esp_timer_get_time() + kProtocolRecoveryIntervalUs;
    if (protocol_->IsAudioChannelOpened()) {
        protocol_reopen_pending_ = false;
        SetListeningMode(static_cast<ListeningMode>(recovery_listening_mode_.load()));
        return;
    }
    SetDeviceState(kDeviceStateConnecting);
    ContinueOpenAudioChannel(
        static_cast<ListeningMode>(recovery_listening_mode_.load()));
}

void Application::HandleStartListeningEvent() {
    auto state = GetDeviceState();
    
    if (state == kDeviceStateActivating) {
        SetDeviceState(kDeviceStateIdle);
        return;
    } else if (state == kDeviceStateWifiConfiguring) {
        audio_service_.EnableAudioTesting(true);
        SetDeviceState(kDeviceStateAudioTesting);
        return;
    }

    if (!protocol_) {
        ESP_LOGE(TAG, "Protocol not initialized");
        return;
    }
    
    if (state == kDeviceStateIdle) {
        if (!protocol_->IsAudioChannelOpened()) {
            SetDeviceState(kDeviceStateConnecting);
            // Jadwalkan dulu agar perubahan status diproses lebih dulu untuk pembaruan antarmuka
            Schedule([this]() {
                ContinueOpenAudioChannel(kListeningModeManualStop);
            });
            return;
        }
        SetListeningMode(kListeningModeManualStop);
    } else if (state == kDeviceStateSpeaking) {
        AbortSpeaking(kAbortReasonNone);
        SetListeningMode(kListeningModeManualStop);
    }
}

void Application::HandleStopListeningEvent() {
    auto state = GetDeviceState();
    
    if (state == kDeviceStateAudioTesting) {
        audio_service_.EnableAudioTesting(false);
        SetDeviceState(kDeviceStateWifiConfiguring);
        return;
    } else if (state == kDeviceStateListening) {
        if (protocol_) {
            protocol_->SendStopListening();
        }
        SetDeviceState(kDeviceStateIdle);
    }
}

void Application::HandleWakeWordDetectedEvent() {
    if (!protocol_) {
        return;
    }

    auto state = GetDeviceState();
    auto wake_word = audio_service_.GetLastWakeWord();
    ESP_LOGI(TAG, "Wake word detected: %s (state: %d)", wake_word.c_str(), (int)state);

    if (state == kDeviceStateIdle) {
        ServoController::GetInstance().WakeGreeting();
        audio_service_.EncodeWakeWord();
        auto wake_word = audio_service_.GetLastWakeWord();

        SetDeviceState(kDeviceStateConnecting);
        if (!protocol_->IsAudioChannelOpened()) {
            // Jadwalkan dulu agar perubahan status diproses lebih dulu untuk pembaruan antarmuka,
            // lalu lanjut ke fungsi pembukaan kanal audio yang bisa tertahan sekitar 1 detik
            Schedule([this, wake_word]() {
                ContinueWakeWordInvoke(wake_word);
            });
            return;
        }
        // Kanal sudah terbuka, tetapi status tetap harus connecting agar proses lanjutan tidak dibuang.
        Schedule([this, wake_word]() {
            ContinueWakeWordInvoke(wake_word);
        });
    } else if (state == kDeviceStateSpeaking) {
        // Peristiwa kata pemicu bisa datang terlambat atau salah deteksi saat pengeras suara aktif.
        // Jangan panggil AbortSpeaking di sini agar jawaban AI tidak terpotong sendiri.
        ESP_LOGW(TAG, "Wake word diabaikan saat AI sedang berbicara: %s", wake_word.c_str());
        audio_service_.EnableWakeWordDetection(false);
    } else if (state == kDeviceStateListening) {
        AbortSpeaking(kAbortReasonWakeWordDetected);
        // Kosongkan antrian kirim agar tidak ada sisa data yang terkirim ke peladen
        while (audio_service_.PopPacketFromSendQueue());

        protocol_->SendStartListening(GetDefaultListeningMode());
        audio_service_.ResetDecoder();
        audio_service_.PlaySound(Lang::Sounds::OGG_POPUP);
        // Aktifkan lagi deteksi kata pemicu karena sebelumnya berhenti saat proses deteksi
        audio_service_.EnableWakeWordDetection(true);
    } else if (state == kDeviceStateActivating) {
        // Ulangi pemeriksaan aktivasi jika kata pemicu terdeteksi saat aktivasi berlangsung
        SetDeviceState(kDeviceStateIdle);
    }
}

void Application::ContinueWakeWordInvoke(const std::string& wake_word) {
    // Periksa status sekali lagi jika berubah saat proses penjadwalan
    if (GetDeviceState() != kDeviceStateConnecting) {
        return;
    }

    auto& board = Board::GetInstance();
    board.SetPowerSaveLevel(PowerSaveLevel::PERFORMANCE);

    if (!protocol_->IsAudioChannelOpened()) {
        if (!protocol_->OpenAudioChannel()) {
            audio_service_.EnableWakeWordDetection(true);
            SetDeviceState(kDeviceStateIdle);
            return;
        }
    }

    ESP_LOGI(TAG, "Wake word detected: %s", wake_word.c_str());
#if CONFIG_SEND_WAKE_WORD_DATA
    // Enkode lalu kirim data kata pemicu ke peladen
    while (auto packet = audio_service_.PopWakeWordPacket()) {
        protocol_->SendAudio(std::move(packet));
    }
    // Ubah status obrolan menjadi kata pemicu terdeteksi
    protocol_->SendWakeWordDetected(wake_word);

    // Atur penanda agar suara penanda diputar setelah status berubah menjadi mendengarkan
    play_popup_on_listening_ = true;
    SetListeningMode(GetDefaultListeningMode());
#else
    // Atur penanda agar suara penanda diputar setelah status berubah menjadi mendengarkan
    // Pemanggilan PlaySound di sini akan dibersihkan oleh ResetDecoder di EnableVoiceProcessing
    play_popup_on_listening_ = true;
    SetListeningMode(GetDefaultListeningMode());
#endif
}

void Application::HandleStateChangedEvent() {
    DeviceState new_state = state_machine_.GetState();
    clock_ticks_ = 0;

    auto& board = Board::GetInstance();
    auto display = board.GetDisplay();
    auto led = board.GetLed();
    led->OnStateChanged();
    ServoController::GetInstance().SetSpeaking(new_state == kDeviceStateSpeaking);
    
    switch (new_state) {
        case kDeviceStateUnknown:
        case kDeviceStateIdle:
            tts_stream_active_ = false;
            tts_stop_pending_ = false;
            last_tts_audio_us_ = 0;
            ServoController::GetInstance().SetTtsStreamActive(false);
            last_stt_had_servo_command_ = false;
            display->SetStatus(Lang::Strings::STANDBY);
            display->ClearChatMessages();  // Bersihkan pesan lebih dulu
            display->SetEmotion("neutral"); // Lalu atur emosi, karena mode WeChat memeriksa jumlah anak
            audio_service_.EnableVoiceProcessing(false);
            audio_service_.EnableWakeWordDetection(true);
            board.SetPowerSaveLevel(PowerSaveLevel::BALANCED);
            break;
        case kDeviceStateConnecting:
            board.SetPowerSaveLevel(PowerSaveLevel::PERFORMANCE);
            display->SetStatus(Lang::Strings::CONNECTING);
            display->SetEmotion("neutral");
            display->SetChatMessage("system", "");
            break;
        case kDeviceStateListening:
            board.SetPowerSaveLevel(PowerSaveLevel::PERFORMANCE);
            tts_stream_active_ = false;
            tts_stop_pending_ = false;
            ServoController::GetInstance().SetTtsStreamActive(false);
            last_stt_had_servo_command_ = false;
            display->SetStatus(Lang::Strings::LISTENING);
            display->SetEmotion("neutral");

            // Pastikan pemroses audio sedang berjalan.
            if (play_popup_on_listening_ || !audio_service_.IsAudioProcessorRunning()) {
                // Kirim perintah mulai mendengarkan
                protocol_->SendStartListening(listening_mode_);
                audio_service_.EnableVoiceProcessing(true);
            }

#ifdef CONFIG_WAKE_WORD_DETECTION_IN_LISTENING
            // Aktifkan deteksi kata pemicu pada mode mendengarkan sesuai konfigurasi Kconfig
            audio_service_.EnableWakeWordDetection(audio_service_.IsAfeWakeWord());
#else
            // Nonaktifkan deteksi kata pemicu pada mode mendengarkan
            audio_service_.EnableWakeWordDetection(false);
#endif
            
            // Mainkan suara penanda setelah ResetDecoder di EnableVoiceProcessing dipanggil
            if (play_popup_on_listening_) {
                play_popup_on_listening_ = false;
                audio_service_.PlaySound(Lang::Sounds::OGG_POPUP);
            }
            break;
        case kDeviceStateSpeaking:
            display->SetStatus(Lang::Strings::SPEAKING);

            if (listening_mode_ != kListeningModeRealtime) {
                audio_service_.EnableVoiceProcessing(false);
                // Nonaktifkan deteksi kata pemicu saat AI berbicara agar audio jawaban tidak berebut sumber daya.
                audio_service_.EnableWakeWordDetection(false);
            }
            break;
        case kDeviceStateWifiConfiguring:
            audio_service_.EnableVoiceProcessing(false);
            audio_service_.EnableWakeWordDetection(false);
            break;
        default:
            // Tidak perlu melakukan apa-apa
            break;
    }
}

void Application::Schedule(std::function<void()>&& callback) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        main_tasks_.push_back(std::move(callback));
    }
    xEventGroupSetBits(event_group_, MAIN_EVENT_SCHEDULE);
}

void Application::AbortSpeaking(AbortReason reason) {
    ESP_LOGI(TAG, "Abort speaking");
    aborted_ = true;
    if (protocol_) {
        protocol_->SendAbortSpeaking(reason);
    }
}

void Application::SetListeningMode(ListeningMode mode) {
    listening_mode_ = mode;
    SetDeviceState(kDeviceStateListening);
}

ListeningMode Application::GetDefaultListeningMode() const {
    return aec_mode_ == kAecOff ? kListeningModeAutoStop : kListeningModeRealtime;
}

void Application::Reboot() {
    ESP_LOGI(TAG, "Rebooting...");
    // Putuskan kanal audio
    if (protocol_ && protocol_->IsAudioChannelOpened()) {
        protocol_->CloseAudioChannel();
    }
    protocol_.reset();
    audio_service_.Stop();

    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
}

bool Application::UpgradeFirmware(const std::string& url, const std::string& version) {
    auto& board = Board::GetInstance();
    auto display = board.GetDisplay();

    std::string upgrade_url = url;
    std::string version_info = version.empty() ? "(Manual upgrade)" : version;

    if (upgrade_url.empty()) {
        ESP_LOGE(TAG, "Firmware upgrade URL is empty");
        return false;
    }

    // Tutup kanal audio jika sedang terbuka
    if (protocol_ && protocol_->IsAudioChannelOpened()) {
        ESP_LOGI(TAG, "Closing audio channel before firmware upgrade");
        protocol_->CloseAudioChannel();
    }
    ESP_LOGI(TAG, "Starting firmware upgrade from URL: %s", upgrade_url.c_str());

    Alert(Lang::Strings::OTA_UPGRADE, Lang::Strings::UPGRADING, "download", Lang::Sounds::OGG_UPGRADE);
    vTaskDelay(pdMS_TO_TICKS(3000));

    SetDeviceState(kDeviceStateUpgrading);

    std::string message = std::string(Lang::Strings::NEW_VERSION) + version_info;
    display->SetChatMessage("system", message.c_str());

    board.SetPowerSaveLevel(PowerSaveLevel::PERFORMANCE);
    audio_service_.Stop();
    vTaskDelay(pdMS_TO_TICKS(1000));

    bool upgrade_success = Ota::Upgrade(upgrade_url, [this, display](int progress, size_t speed) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d%% %uKB/s", progress, speed / 1024);
        Schedule([display, message = std::string(buffer)]() {
            display->SetChatMessage("system", message.c_str());
        });
    });

    if (!upgrade_success) {
        // Jika upgrade gagal, hidupkan ulang layanan audio lalu lanjutkan
        ESP_LOGE(TAG, "Firmware upgrade failed, restarting audio service and continuing operation...");
        audio_service_.Start(); // Nyalakan ulang layanan audio
        board.SetPowerSaveLevel(PowerSaveLevel::BALANCED); // Pulihkan tingkat hemat daya
        Alert(Lang::Strings::ERROR, Lang::Strings::UPGRADE_FAILED, "circle_xmark", Lang::Sounds::OGG_EXCLAMATION);
        vTaskDelay(pdMS_TO_TICKS(3000));
        SetDeviceState(kDeviceStateIdle);
        return false;
    } else {
        // Jika upgrade berhasil, reboot segera
        ESP_LOGI(TAG, "Firmware upgrade successful, rebooting...");
        display->SetChatMessage("system", "Upgrade successful, rebooting...");
        vTaskDelay(pdMS_TO_TICKS(1000)); // Tunda sebentar agar pesan sempat tampil
        Reboot();
        return true;
    }
}

void Application::WakeWordInvoke(const std::string& wake_word) {
    if (!protocol_) {
        return;
    }

    auto state = GetDeviceState();
    
    if (state == kDeviceStateIdle) {
        audio_service_.EncodeWakeWord();

        SetDeviceState(kDeviceStateConnecting);
        if (!protocol_->IsAudioChannelOpened()) {
            // Jadwalkan dulu agar perubahan status diproses lebih dulu untuk pembaruan UI
            Schedule([this, wake_word]() {
                ContinueWakeWordInvoke(wake_word);
            });
            return;
        }
        // Kanal sudah terbuka, tetapi status tetap harus connecting agar proses lanjutan tidak dibuang.
        Schedule([this, wake_word]() {
            ContinueWakeWordInvoke(wake_word);
        });
    } else if (state == kDeviceStateSpeaking) {
        ESP_LOGW(TAG, "WakeWordInvoke diabaikan saat AI sedang berbicara: %s", wake_word.c_str());
        audio_service_.EnableWakeWordDetection(false);
    } else if (state == kDeviceStateListening) {   
        Schedule([this]() {
            if (protocol_) {
                protocol_->CloseAudioChannel();
            }
        });
    }
}

bool Application::CanEnterSleepMode() {
    if (GetDeviceState() != kDeviceStateIdle) {
        return false;
    }

    if (protocol_ && protocol_->IsAudioChannelOpened()) {
        return false;
    }

    if (!audio_service_.IsIdle()) {
        return false;
    }

    // Sekarang perangkat aman untuk masuk ke mode tidur
    return true;
}

void Application::SendMcpMessage(const std::string& payload) {
    // Selalu jadwalkan agar berjalan di tugas utama demi keamanan utas
    Schedule([this, payload = std::move(payload)]() {
        if (protocol_) {
            protocol_->SendMcpMessage(payload);
        }
        if (mcp_broadcast_callback_) {
            mcp_broadcast_callback_(payload);
        }
    });
}

void Application::RegisterMcpBroadcastCallback(std::function<void(const std::string&)> callback) {
    mcp_broadcast_callback_ = std::move(callback);
}

void Application::SetAecMode(AecMode mode) {
    aec_mode_ = mode;
    Schedule([this]() {
        auto& board = Board::GetInstance();
        auto display = board.GetDisplay();
        switch (aec_mode_) {
        case kAecOff:
            audio_service_.EnableDeviceAec(false);
            display->ShowNotification(Lang::Strings::RTC_MODE_OFF);
            break;
        case kAecOnServerSide:
            audio_service_.EnableDeviceAec(false);
            display->ShowNotification(Lang::Strings::RTC_MODE_ON);
            break;
        case kAecOnDeviceSide:
            audio_service_.EnableDeviceAec(true);
            display->ShowNotification(Lang::Strings::RTC_MODE_ON);
            break;
        }

        // Jika mode AEC berubah, tutup kanal audio
        if (protocol_ && protocol_->IsAudioChannelOpened()) {
            protocol_->CloseAudioChannel();
        }
    });
}

void Application::PlaySound(const std::string_view& sound) {
    audio_service_.PlaySound(sound);
}

void Application::ResetProtocol() {
    Schedule([this]() {
        // Tutup kanal audio jika sedang terbuka
        if (protocol_ && protocol_->IsAudioChannelOpened()) {
            protocol_->CloseAudioChannel();
        }
        // Setel ulang protokol
        protocol_.reset();
    });
}
