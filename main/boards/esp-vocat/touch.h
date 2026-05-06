/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Layar sentuh BSP
 *
 * Berkas ini menyediakan API untuk inisialisasi dasar layar sentuh.
 * Berkas ini berguna bagi pengguna yang ingin memakai layar sentuh tanpa pustaka grafis bawaan LVGL.
 *
 * Untuk inisialisasi LCD standar dengan pustaka grafis LVGL, Anda dapat memanggil fungsi terpadu bsp_display_start().
 */

#pragma once
#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Struktur konfigurasi sentuh BSP
 *
 */
typedef struct {
    void *dummy;    /*!< Disiapkan untuk kebutuhan di masa mendatang. */
} bsp_touch_config_t;

/**
 * @brief Membuat layar sentuh baru
 *
 * Jika ingin membebaskan sumber daya yang dialokasikan fungsi ini, Anda dapat memakai API esp_lcd_touch, misalnya:
 *
 * \code{.c}
 * esp_lcd_touch_del(tp);
 * \endcode
 *
 * @param[in]  config    konfigurasi sentuh
 * @param[out] ret_touch objek layar sentuh esp_lcd_touch
 * @return
 *      - ESP_OK         Jika berhasil
 *      - Selain itu     kegagalan dari esp_lcd_touch
 */
esp_err_t bsp_touch_new(const bsp_touch_config_t *config, esp_lcd_touch_handle_t *ret_touch);

#ifdef __cplusplus
}
#endif
