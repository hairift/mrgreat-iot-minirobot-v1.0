# ESP-BOX-3

## Ringkasan

ESP-BOX-3 adalah papan pengembangan berbasis ESP32-S3 dengan layar LCD 2,4 inci, audio codec, dan dukungan interaksi suara. Implementasi papan ini berada di file [esp_box3_board.cc](./esp_box3_board.cc).

## Fitur Utama

- ESP32-S3 dengan layar LCD ILI9341
- Codec audio untuk masukan dan keluaran suara
- Tombol boot untuk percakapan dan konfigurasi Wi-Fi
- Dukungan gaya tampilan standar maupun gaya emote

## Langkah Konfigurasi

1. Atur target build ke `esp32s3`.
2. Buka `menuconfig`.
3. Pada menu `Xiaozhi Assistant`, pilih tipe papan `ESP BOX 3`.
4. Sesuaikan opsi tampilan dan audio bila diperlukan.

## Kompilasi dan Flash

```bash
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py flash
idf.py monitor
```

## Catatan

- Jika memakai gaya tampilan emote, pastikan aset yang sesuai sudah dikonfigurasi.
- Tombol boot dipakai untuk masuk ke mode konfigurasi Wi-Fi saat perangkat masih dalam tahap awal, dan untuk mengubah status percakapan setelah perangkat siap.
