# Waveshare ESP32-S3-Touch-AMOLED-2.06

Dokumen ini merangkum penggunaan board `ESP32-S3-Touch-AMOLED-2.06` pada proyek ini.

Papan ini memakai ESP32-S3R8 dan ditujukan untuk perangkat wearable bergaya jam tangan. Fitur utamanya mencakup layar AMOLED sentuh 2,06 inci, IMU enam sumbu, RTC, codec audio, dan manajemen daya, sehingga cocok untuk prototipe produk portabel yang membutuhkan antarmuka sentuh dan audio.

## Langkah Singkat

1. Pilih target chip `esp32s3`.
2. Buka `menuconfig`.
3. Pilih tipe board `esp32-s3-touch-amoled-2.06`.
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

- https://www.waveshare.com/esp32-s3-touch-amoled-2.06.htm
