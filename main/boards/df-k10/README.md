# DFRobot K10

## Ringkasan

DFRobot K10 adalah papan berbasis ESP32-S3 dengan layar, audio, LED, dan dukungan kamera. Implementasi utamanya berada di file [df_k10_board.cc](./df_k10_board.cc).

## Fitur Utama

- Tombol fisik untuk percakapan dan pengaturan volume
- Kontrol LED bawaan
- Codec audio dan dukungan kamera
- Dapat dikembangkan untuk interaksi suara dan tampilan lokal

## Langkah Konfigurasi

1. Atur target build ke `esp32s3`.
2. Buka `menuconfig`.
3. Pilih tipe papan sesuai nama `df-k10`.
4. Sesuaikan pengaturan PSRAM, kamera, dan buffer tampilan bila diperlukan.

## Kompilasi dan Flash

```bash
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py flash
idf.py monitor
```

## Catatan

- Jika fitur kamera dipakai, periksa kembali pengaturan ukuran buffer dan format tampilan.
- Pengaturan LED, audio, dan tombol mengikuti source terbaru di folder ini.
