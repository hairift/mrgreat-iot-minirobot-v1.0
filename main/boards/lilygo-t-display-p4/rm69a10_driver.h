/*
 * Deskripsi: rm69a10_driver
 * Penulis: LILYGO_L
 * Tanggal: 2025-07-07 14:23:16
 * Perubahan Terakhir: 2025-10-09 10:36:33
 * Lisensi: GPL 3.0
 */
#pragma once

#include <stdint.h>
#include "soc/soc_caps.h"

#if SOC_MIPI_DSI_SUPPORTED
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_mipi_dsi.h"

/**
 * @brief Perintah inisialisasi panel LCD.
 *
 */
typedef struct
{
    int cmd;               /*<! Perintah LCD yang spesifik */
    const void *data;      /*<! Buffer yang menyimpan data khusus perintah */
    size_t data_bytes;     /*<! Ukuran `data` di memori dalam byte */
    unsigned int delay_ms; /*<! Jeda dalam milidetik setelah perintah ini */
} rm69a10_lcd_init_cmd_t;

/**
 * @brief Konfigurasi vendor panel LCD.
 *
 * @note Struktur ini perlu diberikan ke field `vendor_config` pada `esp_lcd_panel_dev_config_t`.
 *
 */
typedef struct
{
    const rm69a10_lcd_init_cmd_t *init_cmds; /*!< Penunjuk ke larik perintah inisialisasi. Atur ke NULL bila memakai perintah bawaan.
                                              *   Larik sebaiknya dideklarasikan sebagai `static const` dan ditempatkan di luar fungsi.
                                              *   Silakan lihat `vendor_specific_init_default` di berkas sumber.
                                              */
    uint16_t init_cmds_size;                 /*<! Jumlah perintah pada larik di atas */
    struct
    {
        esp_lcd_dsi_bus_handle_t dsi_bus;             /*!< Konfigurasi bus MIPI-DSI */
        const esp_lcd_dpi_panel_config_t *dpi_config; /*!< Konfigurasi panel MIPI-DPI */
        uint8_t lane_num;                             /*!< Jumlah jalur MIPI-DSI, bawaan 2 jika diatur ke 0 */
    } mipi_config;
} rm69a10_vendor_config_t;

/**
 * @brief Membuat panel LCD untuk model RM69A10.
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
esp_err_t esp_lcd_new_panel_rm69a10(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config,
                                    esp_lcd_panel_handle_t *ret_panel);

esp_err_t set_rm69a10_brightness(esp_lcd_panel_t *panel, uint8_t brightness);

#endif
