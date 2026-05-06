#ifndef _CST816X_H_
#define _CST816X_H_

#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "i2c_device.h"
#include <driver/i2c_master.h>
#include <sys/time.h>
#include <array>

#define ES8311_VOL_MIN 0
#define ES8311_VOL_MAX 100

enum class TouchEventType {
    SINGLE_CLICK,    // Kejadian klik tunggal
    DOUBLE_CLICK,    // Kejadian klik ganda
    LONG_PRESS_START,// Kejadian awal tekan lama
    LONG_PRESS_END   // Kejadian akhir tekan lama
};

struct TouchEvent {
    TouchEventType type;  
    int x;                
    int y;               
};

class Cst816x : public I2cDevice {
private:
    struct TouchPoint_t {
        int num = 0; 
        int x = -1;   
        int y = -1;   
    };

    struct TouchThresholdConfig {
        int x;                          // Koordinat X tujuan
        int y;                          // Koordinat Y tujuan
        int64_t single_click_thresh_us; // Durasi maksimum klik tunggal (us)
        int64_t double_click_window_us; // Jendela klik ganda (us)
        int64_t long_press_thresh_us;   // Ambang tekan lama (us)
    };

    const TouchThresholdConfig DEFAULT_THRESHOLD = {
        .x = -1, .y = -1,                  
        .single_click_thresh_us = 120000,  // 150ms
        .double_click_window_us = 240000,  // 150ms
        .long_press_thresh_us = 4000000    // 4000ms
    };

    const std::array<TouchThresholdConfig, 3> TOUCH_THRESHOLD_TABLE = {
        {
            {20, 600, 200000, 240000, 2000000}, // Volume naik
            {40, 600, 200000, 240000, 4000000}, // Tombol boot
            {60, 600, 200000, 240000, 2000000}  // Volume turun
        }
    };

    const TouchThresholdConfig& getThresholdConfig(int x, int y);

    uint8_t* read_buffer_ = nullptr;  
    TouchPoint_t tp_;                 

    bool is_touching_ = false;              
    int64_t touch_start_time_ = 0;          // Waktu mulai sentuhan (us)
    int64_t last_release_time_ = 0;         // Waktu terakhir dilepas (us)
    int click_count_ = 0;                   // Penghitung klik tunggal untuk deteksi klik ganda
    bool long_press_started_ = false;       // Menandai apakah tekan lama sudah terpicu

    bool is_volume_long_pressing_ = false;   // Menandai apakah volume sedang disetel lewat tekan lama
    int volume_long_press_dir_ = 0;          // Arah penyesuaian: 1 naik, -1 turun
    int64_t last_volume_adjust_time_ = 0;    // Waktu penyesuaian volume terakhir (us)
    const int64_t VOL_ADJ_INTERVAL_US = 200000; // Jeda penyesuaian volume (100 ms)
    const int VOL_ADJ_STEP = 5;                // Besar langkah penyesuaian tiap kali

    int64_t getCurrentTimeUs();

public:
    Cst816x(i2c_master_bus_handle_t i2c_bus, uint8_t addr);
    ~Cst816x();

    void InitCst816d();
    void UpdateTouchPoint();
    void resetTouchCounters();
    static void touchpad_daemon(void* param);
    
    const TouchPoint_t& GetTouchPoint() { return tp_; }
};

#endif
