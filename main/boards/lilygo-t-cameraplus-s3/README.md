## LILYGO T-CameraPlus-S3

`T-CameraPlus-S3` adalah modul kamera pintar berbasis `ESP32-S3`. Perangkat ini dilengkapi layar TFT `240x240`, mikrofon digital, speaker, tombol mandiri, pengelola daya, dan slot kartu SD. Di firmware bawaan pabrikan biasanya tersedia antarmuka dasar berbasis `LVGL`.

Repositori perangkat keras resmi:

- [T-CameraPlus-S3](https://github.com/Xinyuan-LilyGO/T-CameraPlus-S3)

## Konfigurasi

Tentukan target kompilasi:

```bash
idf.py set-target esp32s3
```

Buka `menuconfig`:

```bash
idf.py menuconfig
```

Pilih tipe papan:

```text
Xiaozhi Assistant -> Board Type -> LILYGO T-CameraPlus-S3_V1_0_V1_1
atau
Xiaozhi Assistant -> Board Type -> LILYGO T-CameraPlus-S3_V1_2
```

Atur konfigurasi `PSRAM`:

```text
Component config -> ESP PSRAM -> SPI RAM config -> Mode (QUAD/OCT) -> Quad Mode PSRAM
```

Pilih dan atur sensor kamera:

```text
Component config -> Espressif Camera Sensors Configurations -> Camera Sensor Configuration -> Select and Set Camera Sensor -> OV2640 -> Select default output format for DVP interface -> YUV422 240x240 25fps, DVP 8-bit, 20M input
```

## Kompilasi

```bash
idf.py build
```
