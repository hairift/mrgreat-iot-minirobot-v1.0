## Wireless-Tag WTP4C5MP07S

[Wireless-Tag WTP4C5MP07S](https://shop.wireless-tag.com/products/7inch-lcd-touch-screen-1024x600-mipi-smart-displays-wtp4c5mp07s-esp32-lcd-board-used-with-esp32-p4-and-esp32-c5-dev-board) adalah kombinasi dari:

- [Wireless-Tag WT99P4C5-S1](https://en.wireless-tag.com/product-item-66.html) sebagai papan pengembangan `ESP32-P4`
- layar LCD `MIPI DSI` 7 inci `1024x600` tipe `ZX7D00C1060M002A`

## Konfigurasi

### Konfigurasi ESP32-P4

- Atur target kompilasi ke `ESP32-P4`

      idf.py set-target esp32p4

- Buka `menuconfig`

      idf.py menuconfig

- Pilih papan

      Xiaozhi Assistant -> Board Type -> Wireless-Tag WTP4C5MP07S

- Atur `PSRAM`

      Component config -> ESP PSRAM -> PSRAM config -> Try to allocate memories of WiFi and LWIP in SPIRAM firstly -> No

- Pilih target `slave` Wi-Fi

      Component config -> Wi-Fi Remote -> choose slave target -> esp32c5

- Atur buffer Wi-Fi

      Component config -> Wi-Fi Remote -> Wi-Fi configuration -> Max number of WiFi static RX buffers -> 10
      Component config -> Wi-Fi Remote -> Wi-Fi configuration -> Max number of WiFi dynamic RX buffers -> 24
      Component config -> Wi-Fi Remote -> Wi-Fi configuration -> Max number of WiFi static TX buffers -> 10

- Bangun firmware

      idf.py build

### Konfigurasi ESP32-C5

- Flash contoh `slave` dari pustaka `esp-hosted-mcu` untuk target `ESP32-C5`.
- Pastikan versi `esp-hosted-mcu` sama dengan yang dipakai di pustaka `xiaozhi-esp32`.
