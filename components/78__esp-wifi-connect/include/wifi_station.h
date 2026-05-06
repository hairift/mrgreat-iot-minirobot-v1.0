#ifndef _WIFI_STATION_H_
#define _WIFI_STATION_H_

#include <string>
#include <vector>
#include <functional>

#include <esp_event.h>
#include <esp_timer.h>
#include <esp_netif.h>
#include <esp_wifi_types_generic.h>

// Enumerasi tingkat hemat daya Wi-Fi
enum class WifiPowerSaveLevel {
    LOW_POWER,    // Hemat daya maksimum (WIFI_PS_MAX_MODEM)
    BALANCED,     // Hemat daya minimum (WIFI_PS_MIN_MODEM)
    PERFORMANCE,  // Tanpa hemat daya (WIFI_PS_NONE) - daya penuh
};

struct WifiApRecord {
    std::string ssid;
    std::string password;
    int channel;
    wifi_auth_mode_t authmode;
    uint8_t bssid[6];
};

/**
 * WifiStation - Penangan mode stasiun Wi-Fi
 *
 * Kelas ini menangani koneksi ke titik akses Wi-Fi dalam mode stasiun.
 * Catatan: penggerak Wi-Fi harus sudah diinisialisasi sebelum kelas ini dipakai.
 */
class WifiStation {
public:
    WifiStation();
    ~WifiStation();

    // Nonaktifkan copy constructor dan operator assignment
    WifiStation(const WifiStation&) = delete;
    WifiStation& operator=(const WifiStation&) = delete;

    void AddAuth(const std::string &&ssid, const std::string &&password);
    void Start();
    void Stop();
    bool IsConnected();
    bool WaitForConnected(int timeout_ms = 10000);
    int8_t GetRssi();
    std::string GetSsid() const { return ssid_; }
    std::string GetIpAddress() const { return ip_address_; }
    uint8_t GetChannel();
    void SetPowerSaveLevel(WifiPowerSaveLevel level);

    void OnConnect(std::function<void(const std::string& ssid)> on_connect);
    void OnConnected(std::function<void(const std::string& ssid)> on_connected);
    void OnDisconnected(std::function<void(int reason)> on_disconnected);
    void OnScanBegin(std::function<void()> on_scan_begin);
    void SetScanIntervalRange(int min_interval_seconds, int max_interval_seconds);

private:
    EventGroupHandle_t event_group_;
    esp_timer_handle_t timer_handle_ = nullptr;
    esp_event_handler_instance_t instance_any_id_ = nullptr;
    esp_event_handler_instance_t instance_got_ip_ = nullptr;
    esp_netif_t* station_netif_ = nullptr;
    std::string ssid_;
    std::string password_;
    std::string ip_address_;
    int8_t max_tx_power_;
    uint8_t remember_bssid_;
    int reconnect_count_ = 0;
    
    // Backoff eksponensial untuk interval pemindaian
    int scan_min_interval_microseconds_ = 10 * 1000 * 1000;      // Bawaan 10 detik
    int scan_max_interval_microseconds_ = 300 * 1000 * 1000;     // Bawaan 5 menit
    int scan_current_interval_microseconds_ = 10 * 1000 * 1000;  // Interval saat ini
    std::function<void(const std::string& ssid)> on_connect_;
    std::function<void(const std::string& ssid)> on_connected_;
    std::function<void(int reason)> on_disconnected_;
    std::function<void()> on_scan_begin_;
    std::vector<WifiApRecord> connect_queue_;
    bool was_connected_ = false;  // Menandai apakah sebelumnya pernah tersambung

    void HandleScanResult();
    void StartConnect();
    void UpdateScanInterval();  // Backoff eksponensial untuk interval pemindaian
    static void WifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    static void IpEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
};

#endif // _WIFI_STATION_H_
