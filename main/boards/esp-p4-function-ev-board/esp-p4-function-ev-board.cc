#include "wifi_board.h"
#include "audio/codecs/es8311_audio_codec.h"
// Layar
#include "display/display.h"
#include "display/lcd_display.h"
#include "lvgl_theme.h"
// Lampu latar
// PwmBacklight dideklarasikan pada header lampu latar yang ikut dimuat melalui display/lcd_display di tumpukan LVGL

#include "application.h"
#include "button.h"
#include "config.h"
#include "esp_video.h"

#include <esp_log.h>
#include <inttypes.h>
#include <driver/i2c_master.h>
#include <esp_lvgl_port.h>
#include <soc/clk_tree_defs.h>
// Kartu SD
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <driver/sdmmc_host.h>
#include <driver/sdspi_host.h>
// Kendali daya SD melalui LDO bawaan chip
#include "sd_pwr_ctrl_by_on_chip_ldo.h"

// Sertakan berkas vendor MIPI-DSI / LCD (sebagian bisa digantikan pustaka)
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_ldo_regulator.h"
#include "esp_lcd_ek79007.h"
#include "esp_lcd_touch_gt911.h"

// Sertakan pustaka
#include "bsp/esp32_p4_function_ev_board.h"
#include "bsp/touch.h"

#define TAG "ESP32P4FuncEV"

class ESP32P4FunctionEvBoard : public WifiBoard
{
private:
    i2c_master_bus_handle_t codec_i2c_bus_ = nullptr;
    Button boot_button_;
    LcdDisplay *display_ = nullptr;
    esp_lcd_touch_handle_t tp_ = nullptr;
    EspVideo* camera_ = nullptr;

    void InitializeI2cBuses()
    {
        ESP_ERROR_CHECK(bsp_i2c_init());
        codec_i2c_bus_ = bsp_i2c_get_handle();
    }

    // Inisialisasi bus I2C sentuh tidak diperlukan pada board ini karena sudah ditangani di tempat lain
    void InitializeTouchI2cBus()
    {
        // Tidak perlu implementasi tambahan
    }

    void InitializeLCD()
    {
        bsp_display_config_t config = {
            .hdmi_resolution = BSP_HDMI_RES_NONE,
            .dsi_bus = {
                .phy_clk_src = (mipi_dsi_phy_clock_source_t)SOC_MOD_CLK_PLL_F20M,
                .lane_bit_rate_mbps = 1000,
            },
        };

        bsp_lcd_handles_t handles;
        ESP_ERROR_CHECK(bsp_display_new_with_handles(&config, &handles));

        display_ = new MipiLcdDisplay(handles.io, handles.panel, 1024, 600, 0, 0, true, true, false);
    }

    void InitializeButtons()
    {
        boot_button_.OnClick([this]()
                             {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });
    }

    void InitializeTouch()
    {
        ESP_ERROR_CHECK(bsp_touch_new(NULL, &tp_));
    }

    void InitializeSdCard()
    {
        ESP_LOGI(TAG, "Initializing SD card");
        esp_err_t ret = bsp_sdcard_mount();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        } else {
            ESP_LOGI(TAG, "SD card mounted successfully");
        }
    }

    void InitializeCamera()
    {
        ESP_LOGI(TAG, "Initializing camera");

        // Gunakan inisialisasi kamera dari BSP untuk ESP-P4
        bsp_camera_cfg_t camera_cfg = {0};
        esp_err_t ret = bsp_camera_start(&camera_cfg);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize BSP camera: %s", esp_err_to_name(ret));
            ESP_LOGI(TAG, "Attempting alternative camera initialization");

            // Alternatif: inisialisasi EspVideo langsung jika BSP gagal
            // Cara ini memberi kendali lebih rinci terhadap konfigurasi kamera
            static esp_cam_ctlr_dvp_pin_config_t dvp_pin_config = {
                .data_width = CAM_CTLR_DATA_WIDTH_8,
                .data_io = {
                    [0] = BSP_I2C_SDA,  // Gunakan ulang pin I2C jika pin kamera tidak didefinisikan
                    [1] = BSP_I2C_SCL,
                    [2] = GPIO_NUM_NC,
                    [3] = GPIO_NUM_NC,
                    [4] = GPIO_NUM_NC,
                    [5] = GPIO_NUM_NC,
                    [6] = GPIO_NUM_NC,
                    [7] = GPIO_NUM_NC,
                },
                .vsync_io = GPIO_NUM_NC,
                .de_io = GPIO_NUM_NC,
                .pclk_io = GPIO_NUM_NC,
                .xclk_io = GPIO_NUM_NC,
            };

            esp_video_init_sccb_config_t sccb_config = {
                .init_sccb = false,  // Gunakan bus I2C yang sudah ada
                .i2c_handle = codec_i2c_bus_,  // Pakai ulang bus I2C yang sudah aktif
                .freq = 100000,
            };

            esp_video_init_dvp_config_t dvp_config = {
                .sccb_config = sccb_config,
                .reset_pin = GPIO_NUM_NC,
                .pwdn_pin = GPIO_NUM_NC,
                .dvp_pin = dvp_pin_config,
                .xclk_freq = 20000000,  // 20 MHz umum dipakai untuk kamera
            };

            esp_video_init_config_t video_config = {
                .dvp = &dvp_config,
            };

            // Coba buat kamera dengan konfigurasi langsung
            camera_ = new EspVideo(video_config);
            ESP_LOGI(TAG, "Camera initialized with direct configuration");
        } else {
            ESP_LOGI(TAG, "Camera initialized successfully via BSP");
        }
    }

    void InitializeFonts()
    {
        ESP_LOGI(TAG, "Initializing font support");
        // Inisialisasi font ditangani oleh sistem aset
        // Papan ini mendukung pemuatan font dari partisi aset
        // Pastikan font termuat dengan benar melalui pemeriksaan tema
        auto& theme_manager = LvglThemeManager::GetInstance();
        auto current_theme = theme_manager.GetTheme("light");
        if (current_theme != nullptr) {
            auto text_font = current_theme->text_font();
            if (text_font != nullptr && text_font->font() != nullptr) {
                ESP_LOGI(TAG, "Custom font loaded successfully: line_height=%d", text_font->font()->line_height);
            } else {
                ESP_LOGW(TAG, "Custom font not loaded, using built-in font");
            }
        }
    }

public:

    ESP32P4FunctionEvBoard() : boot_button_(0)
    {
        InitializeI2cBuses();
        // Perangkat audio diinisialisasi oleh Es8311AudioCodec
        InitializeLCD();
        InitializeButtons();
        InitializeTouch();
        InitializeSdCard();
        InitializeCamera();
        InitializeFonts();
        GetBacklight()->RestoreBrightness();
    }

    ~ESP32P4FunctionEvBoard()
    {
        // Bersihkan penunjuk layar
        delete display_;
        display_ = nullptr;
        // Lepas kait kartu SD
        esp_err_t ret = bsp_sdcard_unmount();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
        }
        // Jika nanti ada sumber daya lain yang perlu dibersihkan, tambahkan di sini
    }

    virtual AudioCodec *GetAudioCodec() override
    {
        static Es8311AudioCodec audio_codec(
            codec_i2c_bus_, (i2c_port_t)BSP_I2C_NUM, AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            BSP_I2S_MCLK, BSP_I2S_SCLK, BSP_I2S_LCLK, BSP_I2S_DOUT, BSP_I2S_DSIN,
            BSP_POWER_AMP_IO, ES8311_CODEC_DEFAULT_ADDR, true, false);
        return &audio_codec;
    }

    virtual Display *GetDisplay() override { return display_; }

    virtual Backlight *GetBacklight() override
    {
        static PwmBacklight backlight(BSP_LCD_BACKLIGHT, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        return &backlight;
    }

    virtual Camera *GetCamera() override
    {
        return camera_;
    }
};

DECLARE_BOARD(ESP32P4FunctionEvBoard);
