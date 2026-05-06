# Bread Compact WiFi S3CAM

Papan ini berbasis ESP32-S3 CAM dan memakai kamera OV2640. Jalur programnya diturunkan dari varian bread-compact dengan layar LCD dan kamera.

## Catatan Perangkat

Karena kamera memakai cukup banyak pin IO, board ini juga menggunakan pin USB 19 dan 20 milik ESP32-S3. Rujukan pin lengkap mengikuti definisi yang ada di `config.h`.

## Kompilasi Cepat

```bash
python scripts/release.py bread-compact-wifi-s3cam
```

## Kompilasi Manual

Atur target chip:

```bash
idf.py set-target esp32s3
```

Buka konfigurasi:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> bread-compact-wifi-s3cam
```

Kompilasi dan flash:

```bash
idf.py -DBOARD_NAME=bread-compact-wifi-s3cam build flash
```
