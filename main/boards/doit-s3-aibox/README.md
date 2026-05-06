# DOIT S3 AI Box

## Ringkasan

DOIT S3 AI Box adalah papan berbasis ESP32-S3 untuk interaksi suara dengan tombol fisik dan LED. Implementasi utamanya berada di file [doit_s3_aibox.cc](./doit_s3_aibox.cc).

## Fitur Utama

- Input suara berbasis mikrofon PDM
- Tombol fisik untuk percakapan dan volume
- LED status bawaan
- Dukungan kompilasi dan flash standar ESP-IDF

## Langkah Konfigurasi

1. Atur target build ke `esp32s3`.
2. Buka `menuconfig`.
3. Pilih tipe papan sesuai nama `doit-s3-aibox`.
4. Sesuaikan pengaturan PSRAM dan audio bila diperlukan.

## Kompilasi dan Flash

```bash
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py flash
idf.py monitor
```

## Catatan

- Tombol volume dan tombol percakapan mengikuti pemetaan pada source terbaru.
- Jika ada perbedaan perangkat keras antarrevisi papan, gunakan source sebagai acuan utama.
