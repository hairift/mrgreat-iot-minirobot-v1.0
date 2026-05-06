#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

/**
 * ServoController untuk robot "Mr Great"
 * Pin servo mengikuti konfigurasi board yang aktif.
 * Kepala hanya gerak yaw, yaitu kiri-kanan. Tidak mendukung angguk naik-turun.
 * Tangan kanan dan kiri bergerak atas-bawah dengan posisi atas, lurus, tengah, dan bawah.
 */

enum class ServoMove {
    NONE = 0,
    // Kepala, hanya yaw dengan posisi dipertahankan
    HEAD_SHAKE,          // gelengkan kepala (animasi, auto-return)
    HEAD_TURN_RIGHT,     // lihat kanan (HOLD)
    HEAD_TURN_LEFT,      // lihat kiri (HOLD)
    HEAD_CENTER,         // lihat depan (HOLD)
    // Tangan kanan
    WAVE_RIGHT_ARM,      // lambaikan tangan kanan (animasi, auto-return)
    RAISE_RIGHT_ARM,     // tangan kanan keatas (HOLD - paling tinggi)
    STRAIGHT_RIGHT_ARM,  // tangan kanan lurus (HOLD - sedang)
    LOWER_RIGHT_ARM,     // tangan kanan kebawah (HOLD)
    // Tangan kiri
    WAVE_LEFT_ARM,       // lambaikan tangan kiri (animasi, auto-return)
    RAISE_LEFT_ARM,      // tangan kiri keatas (HOLD)
    STRAIGHT_LEFT_ARM,   // tangan kiri lurus (HOLD)
    LOWER_LEFT_ARM,      // tangan kiri kebawah (HOLD)
    // Kedua tangan
    WAVE_BOTH_ARMS,      // lambaikan kedua tangan (animasi)
    RAISE_BOTH_ARMS,     // angkat kedua tangan (HOLD)
    // Aksi animasi dengan kembali otomatis
    DANCE,               // joget
    SALAM,               // salam
    HORMAT,              // hormat (HOLD)
    TEPUK_TANGAN,        // tepuk tangan (animasi)
    MENYAPA,             // menyapa / halo (animasi)
    // Sistem
    RESET_POSITION,      // posisi semula
    ENABLE_SERVO,        // servo ON
    DISABLE_SERVO,       // servo OFF
};

class ServoController {
public:
    struct ServoStep {
        int head_angle;
        int rarm_angle;
        int larm_angle;
        int delay_ms;
    };

    struct EmotionSequence {
        const char* emotion;
        const ServoStep* steps;
        int num_steps;
    };

    static ServoController& GetInstance() {
        static ServoController instance;
        return instance;
    }

    void Initialize(int head_gpio, int right_arm_gpio, int left_arm_gpio,
                    bool invert_head = false,
                    bool invert_rarm = false,
                    bool invert_larm = true);

    void Enable();
    void Disable();
    bool IsEnabled() const { return enabled_; }

    void SetEmotion(const char* emotion);
    void ExecuteMove(ServoMove move);
    static ServoMove DetectCommand(const char* text);
    void SetAngle(int channel, int angle_deg, bool invert = false);

private:
    ServoController() = default;
    ~ServoController() = default;

    static void ServoTask(void* arg);
    void InitServo(int channel, int gpio, int init_angle_deg);
    void PlaySteps(const ServoStep* st, int n, bool manual_move = false);
    void ExtendManualOverride(TickType_t duration_ticks);
    bool IsManualOverrideActive() const;
    int  AngleToUs(int angle_deg);

    bool initialized_ = false;
    bool enabled_     = false;

    int head_gpio_    = -1;
    int head_channel_ = 0;
    int rarm_channel_ = 1;
    int larm_channel_ = 2;
    int current_head_angle_ = 90;
    int current_rarm_angle_ = 10;
    int current_larm_angle_ = 10;
    bool invert_head_ = false;
    bool invert_rarm_ = false;
    bool invert_larm_ = true;
    TickType_t manual_override_until_ticks_ = 0;

    TaskHandle_t  task_handle_ = nullptr;
    QueueHandle_t cmd_queue_   = nullptr;
};
