#pragma once

#include "../lvgl_image.h"
#include "gifdec.h"
#include <lvgl.h>
#include <memory>
#include <functional>

/**
 * Implementasi C++ untuk widget GIF LVGL.
 * Menyediakan fungsi animasi GIF dengan memakai pustaka gifdec.
 */
class LvglGif {
public:
    explicit LvglGif(const lv_img_dsc_t* img_dsc);
    virtual ~LvglGif();

    // Implementasi antarmuka LvglImage
    virtual const lv_img_dsc_t* image_dsc() const;

    /**
     * Mulai atau mulai ulang animasi GIF.
     */
    void Start();

    /**
     * Jeda animasi GIF.
     */
    void Pause();

    /**
     * Lanjutkan animasi GIF.
     */
    void Resume();

    /**
     * Hentikan animasi GIF dan kembali ke frame pertama.
     */
    void Stop();

    /**
     * Periksa apakah GIF sedang diputar.
     */
    bool IsPlaying() const;

    /**
     * Periksa apakah GIF berhasil dimuat.
     */
    bool IsLoaded() const;

    /**
     * Ambil jumlah pengulangan.
     */
    int32_t GetLoopCount() const;

    /**
     * Atur jumlah pengulangan.
     */
    void SetLoopCount(int32_t count);

    /**
     * Ambil jeda pengulangan dalam milidetik, yaitu jeda antar loop.
     */
    uint32_t GetLoopDelay() const;

    /**
     * Atur jeda pengulangan dalam milidetik, yaitu jeda antar loop.
     * @param delay_ms Jeda dalam milidetik sebelum memulai loop berikutnya. Nilai 0 berarti tanpa jeda.
     */
    void SetLoopDelay(uint32_t delay_ms);

    /**
     * Ambil dimensi GIF.
     */
    uint16_t width() const;
    uint16_t height() const;

    /**
     * Atur fungsi panggil balik pembaruan frame.
     */
    void SetFrameCallback(std::function<void()> callback);

private:
    // Instance dekoder GIF
    gd_GIF* gif_;
    
    // Deskriptor gambar LVGL
    lv_img_dsc_t img_dsc_;
    
    // Pengatur waktu animasi
    lv_timer_t* timer_;
    
    // Waktu pembaruan bingkai terakhir
    uint32_t last_call_;
    
    // Status animasi
    bool playing_;
    bool loaded_;
    
    // Konfigurasi jeda pengulangan
    uint32_t loop_delay_ms_;      // Jeda antar loop dalam milidetik
    bool loop_waiting_;           // Menandakan apakah sedang menunggu loop berikutnya
    uint32_t loop_wait_start_;    // Penanda waktu saat masa tunggu loop dimulai
    
    // Fungsi panggil balik pembaruan bingkai
    std::function<void()> frame_callback_;
    
    /**
     * Perbarui ke frame berikutnya.
     */
    void NextFrame();
    
    /**
     * Bersihkan sumber daya.
     */
    void Cleanup();
};
