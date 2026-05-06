# Zhengchen 1.54 TFT Wi-Fi

## Ringkasan

Papan ini menggunakan layar TFT 1,54 inci, tombol fisik, dan konektivitas Wi-Fi. Implementasi utamanya berada di file [zhengchen-1.54tft-wifi.cc](./zhengchen-1.54tft-wifi.cc).

## Fitur Utama

- ESP32-S3 dengan layar 240x240
- Tombol boot dan tombol volume
- Manajemen daya dan pemantauan suhu
- Tampilan notifikasi lokal dan mode hemat daya

## Langkah Konfigurasi

1. Atur target build ke `esp32s3`.
2. Buka `menuconfig`.
3. Pilih tipe papan sesuai nama `zhengchen-1.54tft-wifi`.
4. Sesuaikan pengaturan audio, layar, dan manajemen daya bila diperlukan.

## Kompilasi dan Flash

```bash
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py flash
idf.py monitor
```

## Catatan

- Tombol volume dan perilaku hemat daya mengikuti implementasi source terbaru.
- Peringatan suhu tinggi ditangani oleh modul tampilan dan manajemen daya di folder ini.
