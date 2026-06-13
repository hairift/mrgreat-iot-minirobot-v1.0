#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/oled_display.h"
#include "system_reset.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "mcp_server.h"
#include "lamp_controller.h"
#include "servo_controller.h"
#include "servo_mcp_tools.h"
#include "campus_mcp_tools.h"
#include "led/single_led.h"
#include "assets/lang_config.h"
#include "adc_battery_monitor.h"

#include <esp_log.h>
#include <driver/i2c_master.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#ifdef SH1106
#include <esp_lcd_panel_sh1106.h>
#endif

#define TAG "CompactWifiBoard"

class CompactWifiBoard : public WifiBoard {
private:
    i2c_master_bus_handle_t display_i2c_bus_;
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    Display* display_ = nullptr;
    Button boot_button_;
    Button asr_button_;
    AdcBatteryMonitor* adc_battery_monitor_ = nullptr;

    void InitializePowerManager() {
        adc_battery_monitor_ = new AdcBatteryMonitor(BATTERY_ADC_UNIT, BATTERY_ADC_CHANNEL,
            BATTERY_R_UPPER, BATTERY_R_LOWER, BATTERY_CHARGING_GPIO);
    }

    void InitializeDisplayI2c() {
        i2c_master_bus_config_t bus_config = {
            .i2c_port = (i2c_port_t)0,
            .sda_io_num = DISPLAY_SDA_PIN,
            .scl_io_num = DISPLAY_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &display_i2c_bus_));
    }

    void InitializeSsd1306Display() {
        // Tunggu charge pump OLED stabil setelah daya dinyalakan
        ESP_LOGI(TAG, "Waiting for OLED to stabilize...");
        vTaskDelay(pdMS_TO_TICKS(500));

        // Konfigurasi SSD1306
        esp_lcd_panel_io_i2c_config_t io_config = {
            .dev_addr = 0x3C,
            .on_color_trans_done = nullptr,
            .user_ctx = nullptr,
            .control_phase_bytes = 1,
            .dc_bit_offset = 6,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
            .flags = {
                .dc_low_on_data = 0,
                .disable_control_phase = 0,
            },
            .scl_speed_hz = 400 * 1000,
        };

        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c_v2(display_i2c_bus_, &io_config, &panel_io_));

        ESP_LOGI(TAG, "Memasang driver SSD1306");
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = -1;
        panel_config.bits_per_pixel = 1;

        esp_lcd_panel_ssd1306_config_t ssd1306_config = {
            .height = static_cast<uint8_t>(DISPLAY_HEIGHT),
        };
        panel_config.vendor_config = &ssd1306_config;

#ifdef SH1106
        ESP_ERROR_CHECK(esp_lcd_new_panel_sh1106(panel_io_, &panel_config, &panel_));
#else
        ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(panel_io_, &panel_config, &panel_));
#endif
        ESP_LOGI(TAG, "Driver SSD1306 berhasil dipasang");

        // Atur ulang lalu inisialisasi dengan percobaan ulang hingga 3 kali
        bool init_ok = false;
        for (int attempt = 1; attempt <= 3; attempt++) {
            ESP_LOGI(TAG, "OLED init attempt %d/3", attempt);
            ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_));
            vTaskDelay(pdMS_TO_TICKS(100));  // Tunggu setelah reset

            if (esp_lcd_panel_init(panel_) == ESP_OK) {
                init_ok = true;
                ESP_LOGI(TAG, "OLED init success on attempt %d", attempt);
                break;
            }
            ESP_LOGW(TAG, "OLED init failed on attempt %d, retrying...", attempt);
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        if (!init_ok) {
            ESP_LOGE(TAG, "Failed to initialize OLED after 3 attempts");
            display_ = new NoDisplay();
            return;
        }

        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_, false));

        // Nyalakan layar
        ESP_LOGI(TAG, "Menyalakan layar");
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_, true));

        display_ = new OledDisplay(panel_io_, panel_, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
    }

    void InitializeButtons() {
        auto handle_chat_button = [this](const char* source) {
            ESP_LOGI(TAG, "Tombol %s ditekan", source);

            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        };

        boot_button_.OnClick([handle_chat_button]() {
            handle_chat_button("BOOT");
        });

        // Tombol taktil eksternal dibuat sama perilakunya dengan tombol BOOT bawaan.
        asr_button_.OnClick([handle_chat_button]() {
            handle_chat_button("GPIO1");
        });
    }

    // Inisialisasi IoT, servo, dan basis pengetahuan kampus
    void InitializeTools() {
        static LampController lamp(LAMP_GPIO);
        // Inisialisasi tiga servo SG90
        ServoController::GetInstance().Initialize(SERVO_HEAD_GPIO, SERVO_RARM_GPIO, SERVO_LARM_GPIO,
            false,  // tanpa pembalikan arah kepala
            false,  // tanpa pembalikan arah lengan kanan
            true);  // pembalikan arah lengan kiri agar geraknya simetris dengan lengan kanan
        // Daftarkan servo sebagai alat MCP agar AI dapat memanggilnya
        static ServoMcpTools servo_mcp;
        // Daftarkan basis pengetahuan kampus sebagai alat MCP yang selalu aktif
        static CampusMcpTools campus_mcp;
    }

public:
    CompactWifiBoard() :
        boot_button_(BOOT_BUTTON_GPIO),
        asr_button_(ASR_BUTTON_GPIO) {
        InitializePowerManager();
        InitializeDisplayI2c();
        InitializeSsd1306Display();
        InitializeButtons();
        InitializeTools();
    }

    virtual Led* GetLed() override {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }

    virtual AudioCodec* GetAudioCodec() override {
#ifdef AUDIO_I2S_METHOD_SIMPLEX
        // Mikrofon INMP441 dan speaker MAX98357 pada mode I2S simpleks
        static NoAudioCodecSimplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_LRCK, AUDIO_I2S_SPK_GPIO_DOUT,
            AUDIO_I2S_MIC_GPIO_SCK, AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);
        static bool input_gain_configured = false;
        if (!input_gain_configured) {
            // Rekaman INMP441 memiliki ruang puncak sekitar 12 dB; gain 2x menaikkan
            // kejelasan suara tanpa memotong puncak percakapan normal.
            audio_codec.SetInputGain(2.0f);
            input_gain_configured = true;
        }
#else
        static NoAudioCodecDuplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN);
#endif
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }

    virtual bool GetBatteryLevel(int& level, bool& charging, bool& discharging) override {
        if (adc_battery_monitor_ == nullptr) {
            return false;
        }
        charging = adc_battery_monitor_->IsCharging();
        discharging = adc_battery_monitor_->IsDischarging();
        level = adc_battery_monitor_->GetBatteryLevel();
        return true;
    }
};

DECLARE_BOARD(CompactWifiBoard);
