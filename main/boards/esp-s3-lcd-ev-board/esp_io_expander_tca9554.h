/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Ekspander IO ESP: TCA9554
 */

#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "esp_io_expander.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Buat objek ekspander IO TCA9554(A)
 *
 * @param[in]  i2c_bus    Objek bus I2C. Diperoleh dari `i2c_new_master_bus()`
 * @param[in]  dev_addr   Alamat perangkat I2C pada chip. Dapat berupa
 *                        `ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_XXX` atau
 *                        `ESP_IO_EXPANDER_I2C_TCA9554A_ADDRESS_XXX`
 * @param[out] handle_ret Objek ekspander IO yang dibuat
 *
 * @return
 *      - ESP_OK: berhasil, selain itu akan mengembalikan ESP_ERR_xxx
 */
esp_err_t esp_io_expander_new_i2c_tca9554(i2c_master_bus_handle_t i2c_bus, uint32_t dev_addr, esp_io_expander_handle_t *handle_ret);

/**
 * @brief Alamat I2C untuk TCA9554
 *
 * Format alamat 8-bit sebagai berikut:
 *
 *                (Alamat slave)
 *      bit7  bit6  bit5  bit4  bit3 bit2 bit1 bit0
 *       0     1     0     0     A2   A1   A0  R/W
 *
 * Empat bit pertama bersifat tetap, sedangkan A2, A1, dan A0 dipilih melalui
 * perangkat keras.
 *
 * Alamat slave 7-bit adalah data yang paling penting bagi pengguna.
 * Contohnya, jika pin A0, A1, dan A2 terhubung ke GND, alamat slave 7-bit-nya
 * adalah 0100000b (0x20). Pengguna dapat memakai
 * `ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000` untuk inisialisasi.
 */
#define ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000    (0x20)
#define ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_001    (0x21)
#define ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_010    (0x22)
#define ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_011    (0x23)
#define ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_100    (0x24)
#define ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_101    (0x25)
#define ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_110    (0x26)
#define ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_111    (0x27)

/**
 * @brief Alamat I2C untuk TCA9554A
 *
 * Format alamat 8-bit sebagai berikut:
 *
 *                (Alamat slave)
 *      bit7  bit6  bit5  bit4  bit3 bit2 bit1 bit0
 *       0     1     1     1     A2   A1   A0  R/W
 *
 * Empat bit pertama bersifat tetap, sedangkan A2, A1, dan A0 dipilih melalui
 * perangkat keras.
 *
 * Alamat slave 7-bit adalah data yang paling penting bagi pengguna.
 * Contohnya, jika pin A0, A1, dan A2 terhubung ke GND, alamat slave 7-bit-nya
 * adalah 0111000b (0x38). Pengguna dapat memakai
 * `ESP_IO_EXPANDER_I2C_TCA9554A_ADDRESS_000` untuk inisialisasi.
 */
#define ESP_IO_EXPANDER_I2C_TCA9554A_ADDRESS_000    (0x38)
#define ESP_IO_EXPANDER_I2C_TCA9554A_ADDRESS_001    (0x39)
#define ESP_IO_EXPANDER_I2C_TCA9554A_ADDRESS_010    (0x3A)
#define ESP_IO_EXPANDER_I2C_TCA9554A_ADDRESS_011    (0x3B)
#define ESP_IO_EXPANDER_I2C_TCA9554A_ADDRESS_100    (0x3C)
#define ESP_IO_EXPANDER_I2C_TCA9554A_ADDRESS_101    (0x3D)
#define ESP_IO_EXPANDER_I2C_TCA9554A_ADDRESS_110    (0x3E)
#define ESP_IO_EXPANDER_I2C_TCA9554A_ADDRESS_111    (0x3F)

#ifdef __cplusplus
}
#endif
