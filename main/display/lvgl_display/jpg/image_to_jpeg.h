// image_to_jpeg.h - antarmuka pengodean efisien untuk konversi gambar ke JPEG
// Implementasi pengodean JPEG yang menghemat sekitar 8 KB SRAM
#pragma once
#include "sdkconfig.h"
#ifndef CONFIG_IDF_TARGET_ESP32

#include <stdint.h>
#include <stddef.h>

#if defined(CONFIG_IDF_TARGET_ESP32P4) || defined(__LINUX_VIDEODEV2_H)
// ESP32-P4 atau unit translasi yang sudah memuat videodev2 langsung memakai definisi resmi.
#include <linux/videodev2.h>
#elif defined(__has_include)
#if __has_include(<linux/videodev2.h>)
// Saat header V4L2 tersedia, pakai definisi resmi sambil menjaga macro ioctl dari lwIP agar tidak bentrok.
#ifdef _IO
#pragma push_macro("_IO")
#undef _IO
#define IMAGE_TO_JPEG_RESTORE_IO 1
#endif
#ifdef _IOR
#pragma push_macro("_IOR")
#undef _IOR
#define IMAGE_TO_JPEG_RESTORE_IOR 1
#endif
#ifdef _IOW
#pragma push_macro("_IOW")
#undef _IOW
#define IMAGE_TO_JPEG_RESTORE_IOW 1
#endif
#ifdef _IOWR
#pragma push_macro("_IOWR")
#undef _IOWR
#define IMAGE_TO_JPEG_RESTORE_IOWR 1
#endif
#include <linux/videodev2.h>
#ifdef IMAGE_TO_JPEG_RESTORE_IO
#pragma pop_macro("_IO")
#undef IMAGE_TO_JPEG_RESTORE_IO
#endif
#ifdef IMAGE_TO_JPEG_RESTORE_IOR
#pragma pop_macro("_IOR")
#undef IMAGE_TO_JPEG_RESTORE_IOR
#endif
#ifdef IMAGE_TO_JPEG_RESTORE_IOW
#pragma pop_macro("_IOW")
#undef IMAGE_TO_JPEG_RESTORE_IOW
#endif
#ifdef IMAGE_TO_JPEG_RESTORE_IOWR
#pragma pop_macro("_IOWR")
#undef IMAGE_TO_JPEG_RESTORE_IOWR
#endif
#else
// Fallback minimum bila header V4L2 tidak tersedia.
#ifndef V4L2_PIX_FMT_RGB565
#define V4L2_PIX_FMT_RGB565 0x50424752  // 'RGBP'
#endif
#ifndef V4L2_PIX_FMT_RGB565X
#define V4L2_PIX_FMT_RGB565X 0x52474250 // 'PRGB'
#endif
#ifndef V4L2_PIX_FMT_RGB24
#define V4L2_PIX_FMT_RGB24 0x33424752   // 'RGB3'
#endif
#ifndef V4L2_PIX_FMT_YUYV
#define V4L2_PIX_FMT_YUYV 0x56595559    // 'YUYV'
#endif
#ifndef V4L2_PIX_FMT_YUV422P
#define V4L2_PIX_FMT_YUV422P 0x36315559 // 'YU16'
#endif
#ifndef V4L2_PIX_FMT_YUV420
#define V4L2_PIX_FMT_YUV420 0x32315559  // 'YU12'
#endif
#ifndef V4L2_PIX_FMT_GREY
#define V4L2_PIX_FMT_GREY 0x59455247    // 'GREY'
#endif
#ifndef V4L2_PIX_FMT_UYVY
#define V4L2_PIX_FMT_UYVY 0x59565955    // 'UYVY'
#endif
#ifndef V4L2_PIX_FMT_JPEG
#define V4L2_PIX_FMT_JPEG 0x4745504A    // 'JPEG'
#endif
#endif
#else
// Fallback minimum untuk compiler lama yang tidak mendukung __has_include.
#ifndef V4L2_PIX_FMT_RGB565
#define V4L2_PIX_FMT_RGB565 0x50424752  // 'RGBP'
#endif
#ifndef V4L2_PIX_FMT_RGB565X
#define V4L2_PIX_FMT_RGB565X 0x52474250 // 'PRGB'
#endif
#ifndef V4L2_PIX_FMT_RGB24
#define V4L2_PIX_FMT_RGB24 0x33424752   // 'RGB3'
#endif
#ifndef V4L2_PIX_FMT_YUYV
#define V4L2_PIX_FMT_YUYV 0x56595559    // 'YUYV'
#endif
#ifndef V4L2_PIX_FMT_YUV422P
#define V4L2_PIX_FMT_YUV422P 0x36315559 // 'YU16'
#endif
#ifndef V4L2_PIX_FMT_YUV420
#define V4L2_PIX_FMT_YUV420 0x32315559  // 'YU12'
#endif
#ifndef V4L2_PIX_FMT_GREY
#define V4L2_PIX_FMT_GREY 0x59455247    // 'GREY'
#endif
#ifndef V4L2_PIX_FMT_UYVY
#define V4L2_PIX_FMT_UYVY 0x59565955    // 'UYVY'
#endif
#ifndef V4L2_PIX_FMT_JPEG
#define V4L2_PIX_FMT_JPEG 0x4745504A    // 'JPEG'
#endif
#endif

typedef uint32_t v4l2_pix_fmt_t;

#ifdef __cplusplus
extern "C"
{
#endif

    // Tipe fungsi panggil balik keluaran JPEG
    // arg: parameter pengguna, index: indeks data saat ini, data: blok data JPEG, len: panjang blok data
    // Nilai balik: jumlah byte yang benar-benar diproses
    typedef size_t (*jpg_out_cb)(void *arg, size_t index, const void *data, size_t len);

    /**
     * @brief Mengonversi format gambar ke JPEG secara efisien
     *
     * Fungsi ini memakai pengode JPEG yang telah dioptimalkan dengan ciri utama:
     * - Menghemat sekitar 8 KB penggunaan SRAM karena variabel statis dipindah ke alokasi heap
     * - Mendukung berbagai format gambar sebagai masukan
     * - Menghasilkan keluaran JPEG berkualitas tinggi
     *
     * @param src       Data gambar sumber
     * @param src_len   Panjang data gambar sumber
     * @param width     Lebar gambar
     * @param height    Tinggi gambar
     * @param format    Format gambar (PIXFORMAT_RGB565, PIXFORMAT_RGB888, dan lainnya)
     * @param quality   Kualitas JPEG (1-100)
     * @param out       Penunjuk keluaran data JPEG yang harus dibebaskan oleh pemanggil
     * @param out_len   Panjang data JPEG keluaran
     *
     * @return true jika berhasil, false jika gagal
     */
    bool image_to_jpeg(uint8_t *src, size_t src_len, uint16_t width, uint16_t height,
                       v4l2_pix_fmt_t format, uint8_t quality, uint8_t **out, size_t *out_len);

    /**
     * @brief Mengonversi format gambar ke JPEG dalam versi fungsi panggil balik
     *
     * Menggunakan fungsi panggil balik untuk menangani data keluaran JPEG, cocok untuk transmisi aliran atau pemrosesan bertahap:
     * - Menghemat sekitar 8 KB penggunaan SRAM karena variabel statis dipindah ke alokasi heap
     * - Mendukung keluaran berbasis aliran tanpa perlu menyiapkan buffer besar lebih dulu
     * - Memproses data JPEG per blok melalui fungsi panggil balik
     *
     * @param src       Data gambar sumber
     * @param src_len   Panjang data gambar sumber
     * @param width     Lebar gambar
     * @param height    Tinggi gambar
     * @param format    Format gambar
     * @param quality   Kualitas JPEG (1-100)
     * @param cb        Fungsi panggil balik keluaran
     * @param arg       Parameter pengguna yang diteruskan ke fungsi panggil balik
     *
     * @return true jika berhasil, false jika gagal
     */
    bool image_to_jpeg_cb(uint8_t *src, size_t src_len, uint16_t width, uint16_t height,
                          v4l2_pix_fmt_t format, uint8_t quality, jpg_out_cb cb, void *arg);

#ifdef __cplusplus
}
#endif

#endif // ndef CONFIG_IDF_TARGET_ESP32
