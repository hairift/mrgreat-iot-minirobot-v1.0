#ifndef DUAL_NETWORK_BOARD_H
#define DUAL_NETWORK_BOARD_H

#include "board.h"
#include "wifi_board.h"
#include "ml307_board.h"
#include <memory>

// Enum untuk jenis jaringan yang tersedia.
enum class NetworkType {
    WIFI,
    ML307
};

// Board dua jaringan yang dapat berpindah antara Wi-Fi dan ML307.
class DualNetworkBoard : public Board {
private:
    // Simpan board aktif saat ini menggunakan penunjuk kelas dasar.
    std::unique_ptr<Board> current_board_;
    NetworkType network_type_ = NetworkType::ML307;  // Nilai bawaan menggunakan ML307.

    // Konfigurasi pin untuk ML307.
    gpio_num_t ml307_tx_pin_;
    gpio_num_t ml307_rx_pin_;
    gpio_num_t ml307_dtr_pin_;

    // Muat jenis jaringan dari Settings.
    NetworkType LoadNetworkTypeFromSettings(int32_t default_net_type);

    // Simpan jenis jaringan ke Settings.
    void SaveNetworkTypeToSettings(NetworkType type);

    // Inisialisasi board sesuai jaringan yang sedang aktif.
    void InitializeCurrentBoard();

public:
    DualNetworkBoard(gpio_num_t ml307_tx_pin, gpio_num_t ml307_rx_pin, gpio_num_t ml307_dtr_pin = GPIO_NUM_NC, int32_t default_net_type = 1);
    virtual ~DualNetworkBoard() = default;

    // Ganti jenis jaringan.
    void SwitchNetworkType();

    // Ambil jenis jaringan yang aktif saat ini.
    NetworkType GetNetworkType() const { return network_type_; }

    // Ambil referensi board yang sedang aktif.
    Board& GetCurrentBoard() const { return *current_board_; }

    // Implementasi ulang antarmuka Board.
    virtual std::string GetBoardType() override;
    virtual void StartNetwork() override;
    virtual void SetNetworkEventCallback(NetworkEventCallback callback) override;
    virtual NetworkInterface* GetNetwork() override;
    virtual const char* GetNetworkStateIcon() override;
    virtual void SetPowerSaveLevel(PowerSaveLevel level) override;
    virtual std::string GetBoardJson() override;
    virtual std::string GetDeviceStatusJson() override;
};

#endif // DUAL_NETWORK_BOARD_H
