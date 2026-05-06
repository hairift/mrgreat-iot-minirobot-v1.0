# Electron Bot

## Ringkasan

Electron Bot adalah papan robot meja dengan layar bundar dan aktuator gerak. Implementasi utamanya berada di file [electron_bot.cc](./electron_bot.cc), sedangkan logika gerak tambahan berada di file sekitar direktori ini.

## Fitur Utama

- ESP32-S3 dengan layar GC9A01
- Pengendali robot dan tampilan emote khusus
- Tombol boot untuk konfigurasi dan percakapan
- Manajemen daya bawaan

## Langkah Konfigurasi

1. Atur target build ke `esp32s3`.
2. Buka `menuconfig`.
3. Pilih tipe papan sesuai nama `electron-bot`.
4. Tinjau pengaturan tampilan, audio, dan gerakan bila diperlukan.

## Kompilasi dan Flash

```bash
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py flash
idf.py monitor
```

## Catatan

- Logika ekspresi, gerakan, dan tampilan berada di beberapa file dalam folder ini, jadi perubahan perilaku robot sebaiknya diperiksa bersama.
- Tombol boot dipakai untuk masuk ke mode konfigurasi Wi-Fi saat startup dan untuk mengubah status percakapan setelah perangkat siap.
