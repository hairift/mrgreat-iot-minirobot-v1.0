# AtomS3R CAM M12 Echo Base

## Ringkasan

Papan ini menggabungkan modul AtomS3R CAM atau AtomS3R M12 dengan Echo Base untuk kebutuhan suara dan kamera. Implementasi utamanya berada di file [atoms3r_cam_m12_echo_base.cc](./atoms3r_cam_m12_echo_base.cc).

## Fitur Utama

- ESP32-S3 dengan dukungan kamera
- Basis audio untuk input dan output suara
- Cocok untuk interaksi suara tanpa layar
- Dapat dipakai bersama fitur kamera jika protokol dan partisi sudah sesuai

## Langkah Konfigurasi

1. Atur target build ke `esp32s3`.
2. Buka `menuconfig`.
3. Pilih tipe papan sesuai nama `atoms3r-cam-m12-echo-base`.
4. Jika fitur kamera ingin dipakai, pastikan pengaturan partisi dan protokol sudah sesuai kebutuhan perangkat.

## Kompilasi dan Flash

```bash
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py flash
idf.py monitor
```

## Catatan

- Papan ini tidak berfokus pada antarmuka layar, jadi pemeriksaan log serial tetap penting saat pengujian.
- Pastikan ukuran partisi dan konfigurasi flash selaras dengan perangkat keras yang dipakai.
