#include "adc_battery_monitor.h"
#include "settings.h"

#include <esp_log.h>

namespace {

constexpr uint16_t kStartupHoldSamples = 6;
constexpr uint16_t kPersistAfterSamples = 30;
constexpr uint8_t kDisplayHysteresisPercent = 3;
constexpr uint8_t kMaxDisplayStepPercent = 2;

uint8_t ClampBatteryPercent(float level) {
    if (level < 0.0f) {
        return 0;
    }
    if (level > 100.0f) {
        return 100;
    }
    return static_cast<uint8_t>(level + 0.5f);
}

}  // namespace

AdcBatteryMonitor::AdcBatteryMonitor(adc_unit_t adc_unit, adc_channel_t adc_channel,
                                     float upper_resistor, float lower_resistor,
                                     gpio_num_t charging_pin)
    : charging_pin_(charging_pin) {
    // Inisialisasi pin deteksi pengisian jika board menyediakannya.
    if (charging_pin_ != GPIO_NUM_NC) {
        gpio_config_t gpio_cfg = {
            .pin_bit_mask = 1ULL << charging_pin,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&gpio_cfg));
    }

    // Inisialisasi estimasi baterai dari pembagi tegangan ADC.
    adc_battery_estimation_t adc_cfg = {
        .internal = {
            .adc_unit = adc_unit,
            .adc_bitwidth = ADC_BITWIDTH_DEFAULT,
            .adc_atten = ADC_ATTEN_DB_12,
        },
        .adc_channel = adc_channel,
        .upper_resistor = upper_resistor,
        .lower_resistor = lower_resistor,
    };

    if (charging_pin_ != GPIO_NUM_NC) {
        adc_cfg.charging_detect_cb = [](void *user_data) -> bool {
            AdcBatteryMonitor *self = static_cast<AdcBatteryMonitor *>(user_data);
            return gpio_get_level(self->charging_pin_) == 1;
        };
        adc_cfg.charging_detect_user_data = this;
    } else {
        // Tanpa pin deteksi, pustaka memakai estimasi perangkat lunak.
        adc_cfg.charging_detect_cb = nullptr;
        adc_cfg.charging_detect_user_data = nullptr;
    }

    adc_battery_estimation_handle_ = adc_battery_estimation_create(&adc_cfg);
    if (adc_battery_estimation_handle_ == nullptr) {
        ESP_LOGE("AdcBatteryMonitor", "Gagal membuat pembaca baterai ADC");
        return;
    }

    Settings settings("battery", false);
    int saved_level = settings.GetInt("level", -1);
    if (saved_level >= 0 && saved_level <= 100) {
        saved_level_ = static_cast<uint8_t>(saved_level);
        last_persisted_level_ = saved_level_;
        has_saved_level_ = true;
        filtered_level_ = static_cast<float>(saved_level_);
        displayed_level_ = saved_level_;
    }

    // Periksa perubahan status pengisian setiap detik.
    esp_timer_create_args_t timer_cfg = {
        .callback = [](void *arg) {
            AdcBatteryMonitor *self = static_cast<AdcBatteryMonitor *>(arg);
            self->CheckBatteryStatus();
        },
        .arg = this,
        .name = "adc_battery_monitor",
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_cfg, &timer_handle_));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handle_, 1000000));
}

AdcBatteryMonitor::~AdcBatteryMonitor() {
    if (adc_battery_estimation_handle_ != nullptr) {
        ESP_ERROR_CHECK(adc_battery_estimation_destroy(adc_battery_estimation_handle_));
    }

    if (timer_handle_ != nullptr) {
        esp_timer_stop(timer_handle_);
        esp_timer_delete(timer_handle_);
    }
}

bool AdcBatteryMonitor::IsCharging() {
    // Tanpa jalur status TP4056, firmware tidak boleh menebak pengisian dari perubahan ADC.
    if (charging_pin_ == GPIO_NUM_NC) {
        return false;
    }

    if (adc_battery_estimation_handle_ != nullptr) {
        bool is_charging = false;
        esp_err_t err = adc_battery_estimation_get_charging_state(adc_battery_estimation_handle_, &is_charging);
        if (err == ESP_OK) {
            return is_charging;
        }
    }

    return gpio_get_level(charging_pin_) == 1;
}

bool AdcBatteryMonitor::IsDischarging() {
    return !IsCharging();
}

bool AdcBatteryMonitor::IsValid() const {
    return adc_battery_estimation_handle_ != nullptr;
}

uint8_t AdcBatteryMonitor::GetBatteryLevel() {
    if (adc_battery_estimation_handle_ == nullptr) {
        return 0;
    }

    float capacity = 0.0f;
    esp_err_t err = adc_battery_estimation_get_capacity(adc_battery_estimation_handle_, &capacity);
    if (err != ESP_OK) {
        return displayed_level_;
    }

    if (capacity < 0.0f) {
        capacity = 0.0f;
    } else if (capacity >= 95.0f) {
        capacity = 100.0f;
    } else if (capacity > 100.0f) {
        capacity = 100.0f;
    }

    // Pembagi tegangan 100K/100K mudah terlihat naik-turun jika kabel panjang,
    // servo aktif, atau step-up berisik. Filter ini sengaja lambat karena baterai
    // Li-ion tidak mungkin berubah puluhan persen hanya dalam beberapa detik.
    if (!has_level_sample_) {
        has_level_sample_ = true;
        level_sample_count_++;

        if (has_saved_level_) {
            filtered_level_ = (static_cast<float>(saved_level_) * 0.80f) + (capacity * 0.20f);
            displayed_level_ = saved_level_;
            return displayed_level_;
        }

        filtered_level_ = capacity;
        displayed_level_ = ClampBatteryPercent(filtered_level_);
        return displayed_level_;
    }

    level_sample_count_++;
    constexpr float kFilterAlpha = 0.08f;
    filtered_level_ = (filtered_level_ * (1.0f - kFilterAlpha)) + (capacity * kFilterAlpha);

    if (capacity >= 95.0f) {
        filtered_level_ = 100.0f;
        displayed_level_ = 100;
        return displayed_level_;
    }

    // Saat baru menyala, tahan nilai tersimpan sebentar agar tampilan tidak lompat
    // dari 80-an ke 60-an hanya karena sampel ADC pertama masih belum stabil.
    if (has_saved_level_ && level_sample_count_ <= kStartupHoldSamples) {
        return displayed_level_;
    }

    uint8_t candidate = ClampBatteryPercent(filtered_level_);
    int diff = static_cast<int>(candidate) - static_cast<int>(displayed_level_);
    if (diff >= kDisplayHysteresisPercent || diff <= -kDisplayHysteresisPercent) {
        if (diff > kMaxDisplayStepPercent) {
            candidate = static_cast<uint8_t>(displayed_level_ + kMaxDisplayStepPercent);
        } else if (diff < -static_cast<int>(kMaxDisplayStepPercent)) {
            candidate = static_cast<uint8_t>(displayed_level_ - kMaxDisplayStepPercent);
        }
        displayed_level_ = candidate;
    }
    PersistBatteryLevelIfNeeded();
    return displayed_level_;
}

void AdcBatteryMonitor::OnChargingStatusChanged(std::function<void(bool)> callback) {
    on_charging_status_changed_ = callback;
}

void AdcBatteryMonitor::CheckBatteryStatus() {
    bool new_charging_status = IsCharging();
    if (new_charging_status != is_charging_) {
        is_charging_ = new_charging_status;
        if (on_charging_status_changed_) {
            on_charging_status_changed_(is_charging_);
        }
    }
}

void AdcBatteryMonitor::PersistBatteryLevelIfNeeded() {
    if (level_sample_count_ < kPersistAfterSamples) {
        return;
    }

    int diff = static_cast<int>(displayed_level_) - static_cast<int>(last_persisted_level_);
    if (diff < 0) {
        diff = -diff;
    }
    if (diff < 4) {
        return;
    }

    Settings settings("battery", true);
    settings.SetInt("level", displayed_level_);
    last_persisted_level_ = displayed_level_;
}
