#pragma once

#include "esp_lcd_panel_vendor.h"

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
} gc9d01n_lcd_init_cmd_t;

/**
 * @brief Konfigurasi vendor panel LCD.
 *
 * @note  Struktur ini perlu diberikan ke field `vendor_config` pada `esp_lcd_panel_dev_config_t`.
 *
 */
typedef struct {
    const gc9d01n_lcd_init_cmd_t *init_cmds;     /*!< Penunjuk ke larik perintah inisialisasi. Isi NULL jika memakai perintah bawaan.
                                                 *   Larik sebaiknya dideklarasikan sebagai `static const` dan ditempatkan di luar fungsi.
                                                 *   Lihat `vendor_specific_init_default` pada file sumber.
                                                 */
    uint16_t init_cmds_size;                    /*<! Jumlah perintah pada larik di atas */
} gc9d01n_vendor_config_t;

/**
 * @brief Membuat panel LCD untuk model GC9D01N.
 *
 * @note  Inisialisasi khusus vendor bisa berbeda antar pabrikan, jadi urutan inisialisasinya perlu dikonfirmasi ke pemasok LCD.
 *
 * @param[in] io Objek IO panel LCD
 * @param[in] panel_dev_config Konfigurasi umum perangkat panel
 * @param[out] ret_panel Objek panel LCD yang dihasilkan
 * @return
 *          - ESP_ERR_INVALID_ARG   jika parameter tidak valid
 *          - ESP_ERR_NO_MEM        jika memori tidak cukup
 *          - ESP_OK                jika berhasil
 */
esp_err_t esp_lcd_new_panel_gc9d01n(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel);

/**
 * @brief Struktur konfigurasi bus panel LCD.
 *
 * @param[in] sclk Nomor pin clock SPI
 * @param[in] mosi Nomor pin MOSI SPI
 * @param[in] max_trans_sz Ukuran transfer maksimum dalam byte
 *
 */
#define GC9D01N_PANEL_BUS_SPI_CONFIG(sclk, mosi, max_trans_sz)   \
    {                                                           \
        .mosi_io_num = mosi,                                    \
        .miso_io_num = -1,                                      \
        .sclk_io_num = sclk,                                    \
        .quadwp_io_num = -1,                                    \
        .quadhd_io_num = -1,                                    \
        .data4_io_num = -1,                                     \
        .data5_io_num = -1,                                     \
        .data6_io_num = -1,                                     \
        .data7_io_num = -1,                                     \
        .max_transfer_sz = max_trans_sz,                        \
        .flags = 0,                                             \
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,               \
        .intr_flags = 0                                         \
    }

/**
 * @brief Struktur konfigurasi IO panel LCD.
 *
 * @param[in] cs Nomor pin chip select SPI
 * @param[in] dc Nomor pin data/perintah SPI
 * @param[in] cb Fungsi callback saat transfer SPI selesai
 * @param[in] cb_ctx Konteks untuk fungsi callback
 *
 */
#define GC9D01N_PANEL_IO_SPI_CONFIG(cs, dc, callback, callback_ctx)  \
    {                                                               \
        .cs_gpio_num = cs,                                          \
        .dc_gpio_num = dc,                                          \
        .spi_mode = 0,                                              \
        .pclk_hz = 80 * 1000 * 1000,                                \
        .trans_queue_depth = 10,                                    \
        .on_color_trans_done = callback,                            \
        .user_ctx = callback_ctx,                                   \
        .lcd_cmd_bits = 8,                                          \
        .lcd_param_bits = 8,                                        \
        .flags = {}                                                 \
    }

#ifdef __cplusplus
}
#endif
