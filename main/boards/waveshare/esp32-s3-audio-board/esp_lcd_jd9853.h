/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief Driver LCD ESP untuk jd9853
 */

#pragma once

#include "hal/spi_ll.h"
#include "esp_lcd_panel_vendor.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Perintah inisialisasi panel LCD.
 *
 */
typedef struct {
    int cmd;                /*<! Perintah LCD yang spesifik */
    const void *data;       /*<! Buffer yang menyimpan data khusus untuk perintah tersebut */
    size_t data_bytes;      /*<! Ukuran `data` di memori, dalam byte */
    unsigned int delay_ms;  /*<! Jeda dalam milidetik setelah perintah ini */
} jd9853_lcd_init_cmd_t;

/**
 * @brief Konfigurasi vendor panel LCD.
 *
 * @note  Struktur ini harus diberikan ke field `vendor_config` pada `esp_lcd_panel_dev_config_t`.
 *
 */
typedef struct {
    const jd9853_lcd_init_cmd_t *init_cmds;     /*!< Penunjuk ke larik perintah inisialisasi. Atur ke NULL jika memakai perintah bawaan.
                                                 *   Larik harus dideklarasikan sebagai `static const` dan diletakkan di luar fungsi.
                                                 *   Lihat `vendor_specific_init_default` pada file sumber.
                                                 */
    uint16_t init_cmds_size;                    /*<! Jumlah perintah pada larik di atas */
} jd9853_vendor_config_t;

/**
 * @brief Membuat panel LCD untuk model jd9853
 *
 * @note  Inisialisasi khusus vendor dapat berbeda antarprodusen, jadi urutan inisialisasi perlu dikonfirmasi ke pemasok LCD.
 *
 * @param[in] io handle IO panel LCD
 * @param[in] panel_dev_config konfigurasi umum perangkat panel
 * @param[out] ret_panel handle panel LCD yang dikembalikan
 * @return
 *          - ESP_ERR_INVALID_ARG   jika parameter tidak valid
 *          - ESP_ERR_NO_MEM        jika memori tidak cukup
 *          - ESP_OK                jika berhasil
 */
esp_err_t esp_lcd_new_panel_jd9853(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel);

/**
 * @brief Struktur konfigurasi bus panel LCD
 *
 * @param[in] sclk nomor pin clock SPI
 * @param[in] mosi nomor pin MOSI SPI
 * @param[in] max_trans_sz ukuran transfer maksimum dalam byte
 *
 */
#define JD9853_PANEL_BUS_SPI_CONFIG(sclk, mosi, max_trans_sz)  \
    {                                                           \
        .sclk_io_num = sclk,                                    \
        .mosi_io_num = mosi,                                    \
        .miso_io_num = -1,                                      \
        .quadhd_io_num = -1,                                    \
        .quadwp_io_num = -1,                                    \
        .max_transfer_sz = max_trans_sz,                        \
    }

/**
 * @brief Struktur konfigurasi IO panel LCD
 *
 * @param[in] cs nomor pin chip select SPI
 * @param[in] dc nomor pin data/perintah SPI
 * @param[in] cb fungsi panggil balik saat transfer SPI selesai
 * @param[in] cb_ctx konteks untuk fungsi panggil balik
 *
 */
#define JD9853_PANEL_IO_SPI_CONFIG(cs, dc, callback, callback_ctx) \
    {                                                               \
        .cs_gpio_num = cs,                                          \
        .dc_gpio_num = dc,                                          \
        .spi_mode = 0,                                              \
        .pclk_hz = 40 * 1000 * 1000,                                \
        .trans_queue_depth = 10,                                    \
        .on_color_trans_done = callback,                            \
        .user_ctx = callback_ctx,                                   \
        .lcd_cmd_bits = 8,                                          \
        .lcd_param_bits = 8,                                        \
    }

#ifdef __cplusplus
}
#endif
