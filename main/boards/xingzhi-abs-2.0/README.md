# Xingzhi ABS 2.0

## Ringkasan

Xingzhi ABS 2.0 adalah papan dengan layar 1,54 inci, tombol fisik, dukungan kartu SD, dan jaringan tambahan sesuai konfigurasi perangkat keras. Implementasi utamanya berada di file [xingzhi-abs-2.0.cc](./xingzhi-abs-2.0.cc).

## Fitur Utama

- ESP32-S3 dengan layar 240x240
- Tombol boot dan tombol volume fisik
- Dukungan kartu SD dan motor getar
- Manajemen daya dan pemantauan baterai

## Langkah Konfigurasi

1. Atur target build ke `esp32s3`.
2. Buka `menuconfig`.
3. Pilih tipe papan sesuai nama `xingzhi-abs-2.0`.
4. Sesuaikan pengaturan audio, penyimpanan, layar, dan jaringan bila diperlukan.

## Kompilasi dan Flash

```bash
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py flash
idf.py monitor
```

## Catatan

- Status kartu SD dan perilaku getar mengikuti implementasi terbaru pada source.
- Tombol fisik menangani percakapan, pengaturan jaringan, dan volume sesuai logika di file implementasi papan ini.
