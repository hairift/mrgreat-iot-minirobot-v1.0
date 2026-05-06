#ifndef BOARD_H
#define BOARD_H

#include <http.h>
#include <web_socket.h>
#include <mqtt.h>
#include <udp.h>
#include <string>
#include <functional>
#include <network_interface.h>

#include "led/led.h"
#include "backlight.h"
#include "camera.h"
#include "assets.h"

/**
 * Kejadian jaringan untuk fungsi panggil balik terpadu.
 */
enum class NetworkEvent {
    Scanning,              // Jaringan sedang dipindai (pemindaian Wi-Fi dan sejenisnya)
    Connecting,            // Jaringan sedang terhubung (data: SSID/nama jaringan)
    Connected,             // Jaringan berhasil terhubung (data: SSID/nama jaringan)
    Disconnected,          // Jaringan terputus
    WifiConfigModeEnter,   // Masuk ke mode konfigurasi WiFi
    WifiConfigModeExit,    // Keluar dari mode konfigurasi WiFi
    // Kejadian khusus modem seluler
    ModemDetecting,        // Mendeteksi modem (baud rate, tipe modul)
    ModemErrorNoSim,       // Kartu SIM tidak terdeteksi
    ModemErrorRegDenied,   // Registrasi jaringan ditolak
    ModemErrorInitFailed,  // Inisialisasi modem gagal
    ModemErrorTimeout      // Waktu operasi habis
};

// Enumerasi level penghematan daya
enum class PowerSaveLevel {
    LOW_POWER,    // Penghematan daya maksimum (konsumsi daya terendah)
    BALANCED,     // Penghematan daya sedang (seimbang)
    PERFORMANCE,  // Tanpa penghematan daya (konsumsi daya maksimum / performa penuh)
};

// Tipe fungsi panggil balik kejadian jaringan (kejadian, data)
// data berisi informasi tambahan seperti SSID untuk kejadian Connecting/Connected
using NetworkEventCallback = std::function<void(NetworkEvent event, const std::string& data)>;

void* create_board();
class AudioCodec;
class Display;
class Board {
private:
    Board(const Board&) = delete; // Nonaktifkan konstruktor salin
    Board& operator=(const Board&) = delete; // Nonaktifkan operator penugasan

protected:
    Board();
    std::string GenerateUuid();

    // Identitas unik perangkat yang dihasilkan oleh perangkat lunak
    std::string uuid_;

public:
    static Board& GetInstance() {
        static Board* instance = static_cast<Board*>(create_board());
        return *instance;
    }

    virtual ~Board() = default;
    virtual std::string GetBoardType() = 0;
    virtual std::string GetUuid() { return uuid_; }
    virtual Backlight* GetBacklight() { return nullptr; }
    virtual Led* GetLed();
    virtual AudioCodec* GetAudioCodec() = 0;
    virtual bool GetTemperature(float& esp32temp);
    virtual Display* GetDisplay();
    virtual Camera* GetCamera();
    virtual NetworkInterface* GetNetwork() = 0;
    virtual void StartNetwork() = 0;
    virtual void SetNetworkEventCallback(NetworkEventCallback callback) { (void)callback; }
    virtual const char* GetNetworkStateIcon() = 0;
    virtual bool GetBatteryLevel(int &level, bool& charging, bool& discharging);
    virtual std::string GetSystemInfoJson();
    virtual void SetPowerSaveLevel(PowerSaveLevel level) = 0;
    virtual std::string GetBoardJson() = 0;
    virtual std::string GetDeviceStatusJson() = 0;
};

#define DECLARE_BOARD(BOARD_CLASS_NAME) \
void* create_board() { \
    return new BOARD_CLASS_NAME(); \
}

#endif // BOARD_H
