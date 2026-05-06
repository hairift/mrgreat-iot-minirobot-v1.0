/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief Driver ESP LCD untuk GC9503
 */

#pragma once

#include <stdint.h>

#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_rgb.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Perintah inisialisasi panel LCD.
 *
 */
typedef struct {
    int cmd;                /*<! Perintah LCD yang dipakai */
    const void *data;       /*<! Buffer yang menyimpan data khusus untuk perintah tersebut */
    size_t data_bytes;      /*<! Ukuran `data` di memori dalam byte */
    unsigned int delay_ms;  /*<! Jeda dalam milidetik setelah perintah ini */
} gc9503_lcd_init_cmd_t;

/**
 * @brief Konfigurasi vendor panel LCD.
 *
 * @note  Struktur ini perlu diberikan ke field `vendor_config` pada `esp_lcd_panel_dev_config_t`.
 *
 */
typedef struct {
    const esp_lcd_rgb_panel_config_t *rgb_config;   /*!< Konfigurasi panel RGB */
    const gc9503_lcd_init_cmd_t *init_cmds;         /*!< Penunjuk ke larik perintah inisialisasi. Isi NULL jika memakai perintah bawaan.
                                                     *   Larik sebaiknya dideklarasikan sebagai `static const` dan ditempatkan di luar fungsi.
                                                     *   Lihat `vendor_specific_init_default` pada file sumber.
                                                     */
    uint16_t init_cmds_size;                        /*<! Jumlah perintah pada larik di atas */
    struct {
        unsigned int mirror_by_cmd: 1;              /*<! Fungsi `mirror()` dijalankan lewat perintah LCD jika bernilai 1.
                                                     *   Jika tidak, fungsi dijalankan lewat perangkat lunak.
                                                     */
        unsigned int auto_del_panel_io: 1;          /*<! Hapus instance panel IO secara otomatis jika bernilai 1. Semua flag `*_by_cmd` menjadi tidak berlaku.
                                                     *   Jika pin panel IO berbagi dengan pin antarmuka RGB lain untuk menghemat GPIO,
                                                     *   setel ke 1 agar panel IO dan pin-pinnya dilepas, kecuali sinyal CS.
                                                     */
    } flags;
} gc9503_vendor_config_t;

/**
 * @brief Membuat panel LCD untuk model GC9503.
 *
 * @note  Saat `auto_del_panel_io` bernilai 1, fungsi ini akan menginisialisasi GC9503 dengan urutan khusus vendor lebih dulu, lalu memanggil `esp_lcd_new_rgb_panel()` untuk membuat panel LCD RGB. Dalam kondisi ini `esp_lcd_panel_init()` hanya menginisialisasi bagian RGB.
 * @note  Saat `auto_del_panel_io` bernilai 0, fungsi ini hanya memanggil `esp_lcd_new_rgb_panel()` untuk membuat panel LCD RGB. Dalam kondisi ini `esp_lcd_panel_init()` akan menginisialisasi GC9503 dan RGB.
 * @note  Inisialisasi khusus vendor bisa berbeda antar pabrikan, jadi urutan inisialisasinya perlu dikonfirmasi ke pemasok LCD.
 *
 * @param[in]  io Objek IO panel LCD
 * @param[in]  panel_dev_config Konfigurasi umum perangkat panel (`vendor_config` dan `rgb_config` wajib ada)
 * @param[out] ret_panel Objek panel LCD yang dihasilkan
 * @return
 *      - ESP_ERR_INVALID_ARG   jika parameter tidak valid
 *      - ESP_OK                jika berhasil
 *      - selain itu            jika gagal
 */
esp_err_t esp_lcd_new_panel_gc9503(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config,
                                   esp_lcd_panel_handle_t *ret_panel);

/**
 * @brief Struktur konfigurasi IO panel SPI 3 kabel.
 *
 * @param[in] line_cfg Konfigurasi jalur SPI
 * @param[in] scl_active_edge Tepi aktif sinyal SCL, 0: tepi naik, 1: tepi turun
 *
 */
#define GC9503_PANEL_IO_3WIRE_SPI_CONFIG(line_cfg, scl_active_edge) \
    {                                                               \
        .line_config = line_cfg,                                    \
        .expect_clk_speed = PANEL_IO_3WIRE_SPI_CLK_MAX,             \
        .spi_mode = scl_active_edge ? 1 : 0,                        \
        .lcd_cmd_bytes = 1,                                         \
        .lcd_param_bytes = 1,                                       \
        .flags = {                                                  \
            .use_dc_bit = 1,                                        \
            .dc_zero_on_data = 0,                                   \
            .lsb_first = 0,                                         \
            .cs_high_active = 0,                                    \
            .del_keep_cs_inactive = 1,                              \
        },                                                          \
    }

/**
 * @brief Struktur timing RGB.
 *
 * @note  refresh_rate = (pclk_hz * data_width) / (h_res + hsync_pulse_width + hsync_back_porch + hsync_front_porch)
 *                                              / (v_res + vsync_pulse_width + vsync_back_porch + vsync_front_porch)
 *                                              / bits_per_pixel
 *        .pclk_hz = 16 * 1000 * 1000,                \
        .h_res = 384,                               \
        .v_res = 960,                               \
        .hsync_pulse_width = 24,                    \
        .hsync_back_porch = 20,                     \
        .hsync_front_porch = 40,                    \
        .vsync_pulse_width = 30,                    \
        .vsync_back_porch = 18,                     \
        .vsync_front_porch = 20,                    \
        .flags.pclk_active_neg = 0,                 \

                .hsync_pulse_width = 24,                    \
        .hsync_back_porch = 20,                     \
        .hsync_front_porch = 40,                    \
        .vsync_pulse_width = 30,                    \
        .vsync_back_porch = 18,                     \
        .vsync_front_porch = 20,                    \

 */

 #define GC9503_800_480_PANEL_60HZ_RGB_TIMING()      \
 {                                               \
     .pclk_hz = 16 * 1000 * 1000,                \
     .h_res = 800,                               \
     .v_res = 480,                               \
     .hsync_pulse_width = 10,                    \
     .hsync_back_porch = 10,                     \
     .hsync_front_porch = 20,                    \
     .vsync_pulse_width = 10,                    \
     .vsync_back_porch = 10,                     \
     .vsync_front_porch = 10,                    \
     .flags = {                                  \
        .hsync_idle_low = 0,                    \
        .vsync_idle_low = 0,                    \
        .de_idle_high = 0,                      \
        .pclk_active_neg = 0,                   \
        .pclk_idle_high = 0,                    \
    },                                          \
 }

#ifdef __cplusplus
}
#endif
