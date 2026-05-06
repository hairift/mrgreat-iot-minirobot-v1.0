#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <esp_timer.h>

#include <string>
#include <mutex>
#include <deque>
#include <memory>

#include "protocol.h"
#include "ota.h"
#include "audio_service.h"
#include "device_state.h"
#include "device_state_machine.h"

// Bit kejadian utama
#define MAIN_EVENT_SCHEDULE             (1 << 0)
#define MAIN_EVENT_SEND_AUDIO           (1 << 1)
#define MAIN_EVENT_WAKE_WORD_DETECTED   (1 << 2)
#define MAIN_EVENT_VAD_CHANGE           (1 << 3)
#define MAIN_EVENT_ERROR                (1 << 4)
#define MAIN_EVENT_ACTIVATION_DONE      (1 << 5)
#define MAIN_EVENT_CLOCK_TICK           (1 << 6)
#define MAIN_EVENT_NETWORK_CONNECTED    (1 << 7)
#define MAIN_EVENT_NETWORK_DISCONNECTED (1 << 8)
#define MAIN_EVENT_TOGGLE_CHAT          (1 << 9)
#define MAIN_EVENT_START_LISTENING      (1 << 10)
#define MAIN_EVENT_STOP_LISTENING       (1 << 11)
#define MAIN_EVENT_STATE_CHANGED        (1 << 12)


enum AecMode {
    kAecOff,
    kAecOnDeviceSide,
    kAecOnServerSide,
};

class Application {
public:
    static Application& GetInstance() {
        static Application instance;
        return instance;
    }
    // Hapus konstruktor salin dan operator penugasan
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    /**
     * Inisialisasi aplikasi.
     * Bagian ini mengatur layar, audio, fungsi panggil balik jaringan, dan komponen lain.
     * Koneksi jaringan dimulai secara asinkron.
     */
    void Initialize();

    /**
     * Jalankan loop event utama.
     * Fungsi ini berjalan di tugas utama dan tidak pernah kembali.
     * Menangani semua kejadian termasuk jaringan, perubahan status, dan interaksi pengguna.
     */
    void Run();

    DeviceState GetDeviceState() const { return state_machine_.GetState(); }
    bool IsVoiceDetected() const { return audio_service_.IsVoiceDetected(); }
    
    /**
     * Minta transisi status.
     * Mengembalikan true jika transisi berhasil.
     */
    bool SetDeviceState(DeviceState state);

    /**
     * Jadwalkan fungsi panggil balik untuk dieksekusi di tugas utama.
     */
    void Schedule(std::function<void()>&& callback);

    /**
     * Tampilkan peringatan dengan status, pesan, emosi, dan suara opsional.
     */
    void Alert(const char* status, const char* message, const char* emotion = "", const std::string_view& sound = "");
    void DismissAlert();

    void AbortSpeaking(AbortReason reason);

    /**
     * Ubah status obrolan secara berbasis kejadian dan aman untuk utas.
     * Mengirim MAIN_EVENT_TOGGLE_CHAT untuk ditangani di Run().
     */
    void ToggleChatState();

    /**
     * Mulai mendengarkan secara berbasis kejadian dan aman untuk utas.
     * Mengirim MAIN_EVENT_START_LISTENING untuk ditangani di Run().
     */
    void StartListening();

    /**
     * Berhenti mendengarkan secara berbasis kejadian dan aman untuk utas.
     * Mengirim MAIN_EVENT_STOP_LISTENING untuk ditangani di Run().
     */
    void StopListening();

    void Reboot();
    void WakeWordInvoke(const std::string& wake_word);
    bool UpgradeFirmware(const std::string& url, const std::string& version = "");
    bool CanEnterSleepMode();
    void SendMcpMessage(const std::string& payload);
    void SetAecMode(AecMode mode);
    AecMode GetAecMode() const { return aec_mode_; }
    void PlaySound(const std::string_view& sound);
    AudioService& GetAudioService() { return audio_service_; }
    
    /**
     * Atur ulang sumber daya protokol dengan aman untuk utas.
     * Dapat dipanggil dari tugas apa pun untuk melepaskan sumber daya yang dialokasikan setelah jaringan terhubung.
     * Ini termasuk menutup saluran audio serta mengatur ulang objek protokol dan OTA.
     */
    void ResetProtocol();

private:
    Application();
    ~Application();

    std::mutex mutex_;
    std::deque<std::function<void()>> main_tasks_;
    std::unique_ptr<Protocol> protocol_;
    EventGroupHandle_t event_group_ = nullptr;
    esp_timer_handle_t clock_timer_handle_ = nullptr;
    DeviceStateMachine state_machine_;
    ListeningMode listening_mode_ = kListeningModeAutoStop;
    AecMode aec_mode_ = kAecOff;
    std::string last_error_message_;
    AudioService audio_service_;
    std::unique_ptr<Ota> ota_;

    bool has_server_time_ = false;
    bool aborted_ = false;
    bool assets_version_checked_ = false;
    bool play_popup_on_listening_ = false;  // Penanda untuk memutar suara penanda setelah status berubah ke mode mendengarkan
    int clock_ticks_ = 0;
    int speaking_idle_ticks_ = 0;
    TaskHandle_t activation_task_handle_ = nullptr;


    // Penangan kejadian
    void HandleStateChangedEvent();
    void HandleToggleChatEvent();
    void HandleStartListeningEvent();
    void HandleStopListeningEvent();
    void HandleNetworkConnectedEvent();
    void HandleNetworkDisconnectedEvent();
    void HandleActivationDoneEvent();
    void HandleWakeWordDetectedEvent();
    void ContinueOpenAudioChannel(ListeningMode mode);
    void ContinueWakeWordInvoke(const std::string& wake_word);

    // Tugas aktivasi yang berjalan di latar belakang
    void ActivationTask();

    // Metode bantu
    void CheckAssetsVersion();
    void CheckNewVersion();
    void InitializeProtocol();
    void ShowActivationCode(const std::string& code, const std::string& message);
    void SetListeningMode(ListeningMode mode);
    ListeningMode GetDefaultListeningMode() const;
    
    // Penangan perubahan status yang dipanggil oleh mesin status
    void OnStateChanged(DeviceState old_state, DeviceState new_state);
};


class TaskPriorityReset {
public:
    TaskPriorityReset(BaseType_t priority) {
        original_priority_ = uxTaskPriorityGet(NULL);
        vTaskPrioritySet(NULL, priority);
    }
    ~TaskPriorityReset() {
        vTaskPrioritySet(NULL, original_priority_);
    }

private:
    BaseType_t original_priority_;
};

#endif // _APPLICATION_H_
