/*
 * Pengelola Wi-Fi - Pengelolaan koneksi Wi-Fi terpadu
 *
 * Keamanan utas:
 * - Semua metode publik aman dipanggil lintas utas
 *   karena dilindungi mutex internal.
 * - Fungsi panggil balik kejadian dipanggil dari tugas kejadian Wi-Fi.
 *
 * Contoh penggunaan:
 *   auto& wifi = WifiManager::GetInstance();
 *
 *   EventGroupHandle_t events = xEventGroupCreate();
 *   wifi.SetEventCallback([events](WifiEvent e) {
 *       if (e == WifiEvent::Connected) xEventGroupSetBits(events, BIT0);
 *       if (e == WifiEvent::ConfigModeExit) xEventGroupSetBits(events, BIT1);
 *   });
 *
 *   WifiManager::Config cfg;
 *   wifi.Initialize(cfg);
 *   wifi.StartStation();
 *   xEventGroupWaitBits(events, BIT0 | BIT1, pdTRUE, pdFALSE, portMAX_DELAY);
 */

#ifndef _WIFI_MANAGER_H_
#define _WIFI_MANAGER_H_

#include <string>
#include <memory>
#include <functional>
#include <mutex>

#include "wifi_station.h"

class WifiStation;
class WifiConfigurationAp;

// Event Wi-Fi
enum class WifiEvent {
    Scanning,          // Mulai memindai jaringan
    Connecting,        // Sedang menyambung ke jaringan (pakai GetSsid() untuk target)
    Connected,         // Berhasil tersambung
    Disconnected,      // Terputus dari jaringan
    ConfigModeEnter,   // Masuk ke mode AP konfigurasi
    ConfigModeExit,    // Keluar dari mode AP konfigurasi
};

// Konfigurasi
struct WifiManagerConfig {
    std::string ssid_prefix = "ESP32";    // Awalan SSID untuk mode AP
    bool append_mac_suffix = true;        // Tambahkan 4 digit akhir MAC agar SSID unik
    std::string language = "zh-CN";       // Bahasa antarmuka web

    // Interval pindai mode stasiun dengan backoff eksponensial
    int station_scan_min_interval_seconds = 10;   // Interval awal pemindaian
    int station_scan_max_interval_seconds = 300;  // Interval maksimum (5 menit)
};

/**
 * WifiManager - Objek tunggal untuk pengelolaan Wi-Fi
 */
class WifiManager {
public:
    static WifiManager& GetInstance();

    // ==================== Siklus hidup ====================

    bool Initialize(const WifiManagerConfig& config = WifiManagerConfig{});
    bool IsInitialized() const;

    // ==================== Mode stasiun ====================

    void StartStation();   // Non-blocking, otomatis menghentikan AP konfigurasi jika aktif
    void StopStation();    // Non-blocking

    bool IsConnected() const;
    std::string GetSsid() const;
    std::string GetIpAddress() const;
    int GetRssi() const;
    int GetChannel() const;
    std::string GetMacAddress() const;

    // ==================== Mode AP konfigurasi ====================

    void StartConfigAp();  // Non-blocking, otomatis menghentikan mode stasiun jika aktif
    void StopConfigAp();   // Non-blocking

    bool IsConfigMode() const;
    std::string GetApSsid() const;
    std::string GetApWebUrl() const;

    // ==================== Daya ====================

    void SetPowerSaveLevel(WifiPowerSaveLevel level);

    // ==================== Event ====================

    void SetEventCallback(std::function<void(WifiEvent, const std::string&)> callback);

    const WifiManagerConfig& GetConfig() const { return config_; }

    WifiManager(const WifiManager&) = delete;
    WifiManager& operator=(const WifiManager&) = delete;

private:
    WifiManager();
    ~WifiManager();

    void NotifyEvent(WifiEvent event, const std::string& data = "");

    WifiManagerConfig config_;
    std::unique_ptr<WifiStation> station_;
    std::unique_ptr<WifiConfigurationAp> config_ap_;

    mutable std::mutex mutex_;
    bool initialized_ = false;
    bool station_active_ = false;
    bool config_mode_active_ = false;

    std::function<void(WifiEvent, const std::string&)> event_callback_;
    mutable std::string mac_address_;
};

#endif // _WIFI_MANAGER_H_
