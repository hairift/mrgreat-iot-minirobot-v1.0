/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief Driver ESP LCD untuk ST7123
 */

 #pragma once

 #include <stdint.h>
 #include "soc/soc_caps.h"
 
 #if SOC_MIPI_DSI_SUPPORTED
 #include "esp_lcd_panel_vendor.h"
 #include "esp_lcd_mipi_dsi.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Perintah inisialisasi panel LCD.
  *
  */
 typedef struct {
     int cmd;                /*<! Perintah LCD yang spesifik */
     const void *data;       /*<! Buffer yang menyimpan data khusus perintah */
     size_t data_bytes;      /*<! Ukuran `data` di memori dalam byte */
     unsigned int delay_ms;  /*<! Jeda dalam milidetik setelah perintah ini */
 } st7123_lcd_init_cmd_t;
 
 /**
  * @brief Konfigurasi vendor panel LCD.
  *
  * @note Struktur ini perlu diberikan ke `esp_lcd_panel_dev_config_t::vendor_config`.
  *
  */
 typedef struct {
     const st7123_lcd_init_cmd_t *init_cmds;       /*!< Penunjuk ke larik perintah inisialisasi. Atur ke NULL bila memakai perintah bawaan.
                                                      *   Larik sebaiknya dideklarasikan sebagai `static const` dan ditempatkan di luar fungsi.
                                                      *   Silakan lihat `vendor_specific_init_default` di berkas sumber.
                                                      */
     uint16_t init_cmds_size;                        /*<! Jumlah perintah pada larik di atas */
     struct {
         esp_lcd_dsi_bus_handle_t dsi_bus;               /*!< Konfigurasi bus MIPI-DSI */
         const esp_lcd_dpi_panel_config_t *dpi_config;   /*!< Konfigurasi panel MIPI-DPI */
         uint8_t  lane_num;                              /*!< Jumlah jalur MIPI-DSI */
     } mipi_config;
 } st7123_vendor_config_t;
 
 /**
  * @brief Membuat panel LCD untuk model ST7123.
  *
  * @note Urutan inisialisasi khusus vendor bisa berbeda antar pabrikan, jadi sebaiknya rujuk ke pemasok LCD.
  *
  * @param[in]  io Objek IO panel LCD
  * @param[in]  panel_dev_config Konfigurasi umum perangkat panel
  * @param[out] ret_panel Objek panel LCD yang dikembalikan
  * @return
  *      - ESP_ERR_INVALID_ARG   jika parameter tidak valid
  *      - ESP_OK                jika berhasil
  *      - Selain itu            jika gagal
  */
 esp_err_t esp_lcd_new_panel_st7123(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config,
                                      esp_lcd_panel_handle_t *ret_panel);
 
 /**
  * @brief Struktur konfigurasi bus MIPI-DSI.
  *
  */
 #define ST7123_PANEL_BUS_DSI_2CH_CONFIG()              \
     {                                                    \
         .bus_id = 0,                                     \
         .num_data_lanes = 2,                             \
         .lane_bit_rate_mbps = 1000,                      \
     }
 
 /**
  * @brief Struktur konfigurasi IO panel MIPI-DBI.
  *
  */
 #define ST7123_PANEL_IO_DBI_CONFIG()  \
     {                                   \
         .virtual_channel = 0,           \
         .lcd_cmd_bits = 8,              \
         .lcd_param_bits = 8,            \
     }
 
 /**
  * @brief Struktur konfigurasi MIPI-DPI.
  *
  * @note refresh_rate = (dpi_clock_freq_mhz * 1000000) / (h_res + hsync_pulse_width + hsync_back_porch + hsync_front_porch)
  *                                                     / (v_res + vsync_pulse_width + vsync_back_porch + vsync_front_porch)
  *
  * @param[in] px_format Format piksel panel
  *
  */
 #define ST7123_800_1280_PANEL_60HZ_DPI_CONFIG(px_format) \
     {                                                      \
         .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,       \
         .dpi_clock_freq_mhz = 80,                          \
         .virtual_channel = 0,                              \
         .pixel_format = px_format,                         \
         .num_fbs = 1,                                      \
         .video_timing = {                                  \
             .h_size = 800,                                 \
             .v_size = 1280,                                \
             .hsync_back_porch = 140,                       \
             .hsync_pulse_width = 40,                       \
             .hsync_front_porch = 40,                       \
             .vsync_back_porch = 16,                        \
             .vsync_pulse_width = 4,                        \
             .vsync_front_porch = 16,                       \
         },                                                 \
         .flags.use_dma2d = true,                           \
     }
 #endif
 
 #ifdef __cplusplus
 }
 #endif
 
