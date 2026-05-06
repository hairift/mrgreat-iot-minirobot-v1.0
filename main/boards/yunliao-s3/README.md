# Yunliao S3

## Ringkasan

Yunliao S3 adalah perangkat berbasis ESP32-S3 dengan layar besar, manajemen daya, dan opsi jaringan tambahan sesuai konfigurasi perangkat keras. Implementasi utama papan ini berada di file [yunliao_s3.cc](./yunliao_s3.cc).

## Fitur Utama

- Layar LCD untuk antarmuka percakapan
- Dukungan audio dan mode hemat daya
- Tombol untuk percakapan, pengaturan, dan fungsi daya
- Integrasi dengan manajemen baterai dan pengisian daya

## Langkah Konfigurasi

1. Atur target build ke `esp32s3`.
2. Buka `menuconfig`.
3. Pilih tipe papan sesuai nama `yunliao-s3`.
4. Sesuaikan pengaturan audio, jaringan, dan hemat daya sesuai kebutuhan perangkat.

## Kompilasi dan Flash

```bash
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py flash
idf.py monitor
```

## Catatan

- Fungsi tombol dan perilaku daya mengikuti implementasi terbaru pada source, sehingga pengujian sesudah flash tetap disarankan.
- Jika perangkat memakai modul jaringan tambahan, pastikan opsi terkait sudah diaktifkan pada konfigurasi.
