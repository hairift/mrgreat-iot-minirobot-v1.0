# Waveshare ESP32-S3 Touch LCD 4.3C

Referensi produk:

- https://www.waveshare.com/esp32-s3-touch-lcd-4.3c.htm

Jika ingin mencoba firmware siap pakai, referensi unduhan lama masih tersedia di:

- https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3C/ESP32-S3-Touch-LCD-4.3C-Xiaozhi.bin

## Menentukan target

```bash
idf.py set-target esp32s3
```

## Membuka konfigurasi

```bash
idf.py menuconfig
```

Pilih tipe board `esp32-s3-touch-lcd-4.3c` pada menu `Xiaozhi Assistant -> Board Type`.

Jika tim menggunakan konfigurasi khusus yang sebelumnya mengandalkan salinan `sdkconfig.4_3c`, pastikan parameter tersebut sudah dipindahkan ke `menuconfig` atau file konfigurasi proyek yang sedang dipakai.

## Kompilasi dan flash

```bash
idf.py build flash monitor
```
