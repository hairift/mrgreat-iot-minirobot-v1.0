#ifndef WIFI_BOARD_H
#define WIFI_BOARD_H

#include "board.h"
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_timer.h>

class WifiBoard : public Board {
protected:
    esp_timer_handle_t connect_timer_ = nullptr;
    bool in_config_mode_ = false;
    bool is_connected_ = false;
    NetworkEventCallback network_event_callback_ = nullptr;

    virtual std::string GetBoardJson() override;

    /**
     * Tangani kejadian jaringan yang dipanggil dari fungsi panggil balik pengelola Wi-Fi.
     * @param event Tipe kejadian jaringan.
     * @param data Data tambahan, misalnya SSID untuk kejadian Connecting atau Connected.
     */
    void OnNetworkEvent(NetworkEvent event, const std::string& data = "");

    /**
     * Mulai percobaan koneksi Wi-Fi.
     */
    void TryWifiConnect();

    /**
     * Masuk ke mode konfigurasi Wi-Fi.
     */
    void StartWifiConfigMode();

    /**
     * Callback saat koneksi Wi-Fi melewati batas waktu.
     */
    static void OnWifiConnectTimeout(void* arg);

public:
    WifiBoard();
    virtual ~WifiBoard();
    
    virtual std::string GetBoardType() override;
    
    /**
     * Mulai koneksi jaringan secara asinkron.
     * Fungsi ini kembali segera. Kejadian jaringan diberitahukan melalui fungsi panggil balik yang diatur oleh SetNetworkEventCallback().
     */
    virtual void StartNetwork() override;
    
    virtual NetworkInterface* GetNetwork() override;
    virtual void SetNetworkEventCallback(NetworkEventCallback callback) override;
    virtual const char* GetNetworkStateIcon() override;
    virtual void SetPowerSaveLevel(PowerSaveLevel level) override;
    virtual AudioCodec* GetAudioCodec() override { return nullptr; }
    virtual std::string GetDeviceStatusJson() override;
    
    /**
     * Masuk ke mode konfigurasi Wi-Fi, aman untuk utas dan dapat dipanggil dari task apa pun.
     */
    void EnterWifiConfigMode();
    
    /**
     * Periksa apakah perangkat sedang berada di mode konfigurasi Wi-Fi.
     */
    bool IsInWifiConfigMode() const;
};

#endif // WIFI_BOARD_H
