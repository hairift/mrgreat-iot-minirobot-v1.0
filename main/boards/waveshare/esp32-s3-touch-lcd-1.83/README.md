# Waveshare ESP32-S3-Touch-LCD-1.83

Dokumen ini merangkum penggunaan board `ESP32-S3-Touch-LCD-1.83` pada proyek ini.

Papan ini memakai ESP32-S3 dan berukuran ringkas sehingga mudah dipakai untuk pengembangan maupun integrasi ke produk jadi. Fitur utamanya meliputi layar LCD sentuh kapasitif 1,83 inci, manajemen daya terintegrasi, sensor enam sumbu, RTC, dan codec audio berdaya rendah.

## Langkah Singkat

1. Pilih target chip `esp32s3`.
2. Buka `menuconfig`.
3. Pilih tipe board `esp32-s3-touch-lcd-1.83`.
4. Build, flash, lalu monitor perangkat.

## Perintah Dasar

```powershell
cd E:\MrGreat-IOT-ESP32\MrGreat-esp32-main
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

## Referensi Perangkat Keras

Halaman produk resmi:

- https://www.waveshare.com/esp32-s3-touch-lcd-1.83.htm
