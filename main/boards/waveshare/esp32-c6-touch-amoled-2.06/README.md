# Waveshare ESP32-C6-Touch-AMOLED-2.06

Dokumen ini merangkum penggunaan board `ESP32-C6-Touch-AMOLED-2.06` pada proyek ini.

Papan ini memakai ESP32-C6 dan dirancang dalam bentuk perangkat wearable. Perangkat kerasnya sudah memadukan layar AMOLED sentuh 2,06 inci, sensor enam sumbu, RTC, codec audio, dan pengelolaan daya, sehingga cocok untuk prototipe perangkat portabel dengan antarmuka sentuh.

## Langkah Singkat

1. Pilih target chip yang sesuai.
2. Buka `menuconfig`.
3. Pilih tipe board `esp32-c6-touch-amoled-2.06`.
4. Build, flash, lalu monitor perangkat.

## Perintah Dasar

```powershell
cd E:\MrGreat-IOT-ESP32\MrGreat-esp32-main
idf.py set-target esp32c6
idf.py menuconfig
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

## Referensi Perangkat Keras

Halaman produk resmi:

- https://www.waveshare.com/esp32-c6-touch-amoled-2.06.htm
