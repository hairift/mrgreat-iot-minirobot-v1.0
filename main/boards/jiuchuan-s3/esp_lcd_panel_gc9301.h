/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "esp_lcd_panel_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Membuat panel LCD untuk model GC9309NA.
 *
 * @param[in] io Objek IO panel LCD
 * @param[in] panel_dev_config Konfigurasi umum perangkat panel
 * @param[out] ret_panel Objek panel LCD yang dihasilkan
 * @return
 *          - ESP_ERR_INVALID_ARG   jika parameter tidak valid
 *          - ESP_ERR_NO_MEM        jika memori tidak cukup
 *          - ESP_OK                jika berhasil
 */
esp_err_t esp_lcd_new_panel_gc9309na(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel);

#ifdef __cplusplus
}
#endif
