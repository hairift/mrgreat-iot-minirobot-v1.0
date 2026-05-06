# ESP-P4-Function-EV-Board

Dokumentasi ini menjelaskan dukungan firmware untuk `ESP-P4-Function-EV-Board`. Koneksi Wi-Fi memakai `ESP-Hosted` melalui modul `ESP32-C6` bawaan, sedangkan layar didukung lewat adaptor LCD `MIPI-DSI` resmi.

## Fitur

- Wi-Fi: `esp_wifi_remote` + `esp_hosted` melalui SDIO dengan koprosesor `ESP32-C6`
- Layar: LCD `MIPI-DSI` 7 inci beresolusi `1024x600`, atau dapat dijalankan tanpa layar
- Audio: codec `ES8311` dengan dukungan speaker dan mikrofon
- Sentuh: pengendali sentuh kapasitif `GT911`
- Kartu SD: dukungan `MicroSD` dalam mode `MMC`
- Kamera: antarmuka `MIPI-CSI` dengan cadangan konfigurasi `DVP` untuk sensor `OV5647` dan `SC2336`
- USB: dukungan host maupun device
- SPIFFS: dukungan sistem berkas flash internal
- Font: dukungan font kustom dan karakter Unicode

## Konfigurasi

Di `menuconfig`, pilih:

```text
Xiaozhi Assistant -> Board Type -> ESP-P4-Function-EV-Board
```

Pastikan opsi berikut aktif. Saat build melalui `config.json`, opsi ini biasanya terpasang otomatis:

- `CONFIG_SLAVE_IDF_TARGET_ESP32C6=y`
- `CONFIG_ESP_HOSTED_P4_DEV_BOARD_FUNC_BOARD=y`
- `CONFIG_ESP_HOSTED_SDIO_HOST_INTERFACE=y`
- `CONFIG_ESP_HOSTED_SDIO_4_BIT_BUS=y`

## Sambungan LCD

Mengacu pada panduan Espressif:

- Hubungkan adaptor LCD konektor `J3` ke konektor `MIPI DSI` papan memakai kabel pita terbalik.
- Hubungkan `RST_LCD` pada adaptor `J6` ke `GPIO27` pada papan `J1`.
- Hubungkan `PWM` pada adaptor `J6` ke `GPIO26` pada papan `J1`.
- Adaptor LCD bisa diberi daya lewat USB-nya sendiri atau dari `5V` dan `GND` papan.

Pin tersebut sudah disiapkan di `config.h` sebagai `PIN_NUM_LCD_RST=GPIO27` dan `DISPLAY_BACKLIGHT_PIN=GPIO26`. Resolusi bawaan diatur ke `1024x600`.

## Kompilasi

```powershell
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
```

Jika build dilakukan melalui skrip rilis, `config.json` pada folder ini akan menambahkan opsi `ESP-Hosted` yang dibutuhkan.
