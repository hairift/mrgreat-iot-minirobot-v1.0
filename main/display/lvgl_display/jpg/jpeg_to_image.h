#include "sdkconfig.h"
#ifndef CONFIG_IDF_TARGET_ESP32

#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Dekode gambar JPEG dari memori menjadi data piksel RGB565 mentah.
 *
 * Fungsi ini mencoba mendekode gambar JPEG dengan akselerasi perangkat keras terlebih dahulu
 * jika tersedia, lalu beralih ke dekoder perangkat lunak bila dekode perangkat keras gagal
 * atau tidak tersedia.
 *
 * @param[in] src Penunjuk ke bitstream JPEG di memori.
 * @param[in] src_len Panjang bitstream JPEG dalam byte.
 * @param[out] out Penunjuk ke penunjuk buffer yang akan diisi dengan data gambar hasil dekode.
 *             Buffer ini dialokasikan secara internal dan WAJIB dibebaskan pemanggil dengan heap_caps_free().
 * @param[out] out_len Penunjuk ke variabel yang akan menerima ukuran data gambar hasil dekode dalam byte.
 * @param[out] width Penunjuk ke variabel yang akan menerima lebar gambar dalam piksel.
 * @param[out] height Penunjuk ke variabel yang akan menerima tinggi gambar dalam piksel.
 * @param[out] stride Penunjuk ke variabel yang akan menerima nilai stride gambar dalam byte.
 *
 * @return ESP_OK bila dekode berhasil.
 * @return ESP_ERR_INVALID_ARG bila parameter tidak valid.
 * @return ESP_ERR_NO_MEM bila alokasi memori gagal.
 * @return ESP_FAIL bila dekode gagal.
 *
 * @attention Pengelolaan memori untuk `*out`:
 *            - Fungsi mengalokasikan memori untuk gambar hasil dekode secara internal.
 *            - Jika berhasil, kepemilikan memori berpindah ke pemanggil dan SEBAIKNYA dibebaskan dengan heap_caps_free().
 *            - Jika gagal, `*out` dijamin bernilai NULL dan tidak perlu dibebaskan.
 *            - Contoh penggunaan:
 *              @code{.c}
 *              uint8_t *image = NULL;
 *              size_t len, width, height;
 *              if (jpeg_to_image(jpeg_data, jpeg_len, &image, &len, &width, &height)) {
 *                  // Gunakan data gambar...
 *                  heap_caps_free(image);  // Penting: gunakan heap_caps_free
 *              }
 *              @endcode
 *
 * @note Ketergantungan konfigurasi:
 *       - Saat CONFIG_XIAOZHI_ENABLE_HARDWARE_JPEG_DECODER aktif, akselerasi perangkat keras dicoba lebih dahulu.
 *       - Jalur perangkat keras dan perangkat lunak sama-sama mengalokasikan memori yang harus dibebaskan dengan heap_caps_free().
 *       - Format gambar hasil dekode selalu RGB565, yaitu 2 byte per piksel.
 *
 * @note Saat memakai dekoder perangkat keras, dimensi gambar hasil dekode dapat disejajarkan ke batas 16 byte.
 *       Untuk gambar terkompresi YUV420 atau YUV422, lebar dan tinggi akan dibulatkan ke atas ke kelipatan 16 terdekat.
 *       Rincian ada di
 *       <https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/jpeg.html#jpeg-decoder-engine>
 *
 */
esp_err_t jpeg_to_image(const uint8_t* src, size_t src_len, uint8_t** out, size_t* out_len, size_t* width,
                        size_t* height, size_t* stride);

#ifdef __cplusplus
}
#endif

#endif  // CONFIG_IDF_TARGET_ESP32
