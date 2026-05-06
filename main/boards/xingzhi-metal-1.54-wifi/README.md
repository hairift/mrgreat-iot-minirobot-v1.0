# Xingzhi Metal 1.54 Wi-Fi

## Ringkasan

Papan ini memakai layar 1,54 inci dan antarmuka sentuh CST816 sebagai pengganti tombol fisik. Implementasi utamanya berada di file [xingzhi-metal-1.54-wifi.cc](./xingzhi-metal-1.54-wifi.cc).

## Fitur Utama

- ESP32-S3 dengan layar 240x240
- Kontrol sentuh untuk percakapan dan volume
- Manajemen daya dengan pemantauan baterai
- Mode tidur ringan dan tidur dalam

## Langkah Konfigurasi

1. Atur target build ke `esp32s3`.
2. Buka `menuconfig`.
3. Pilih tipe papan sesuai nama `xingzhi-metal-1.54-wifi`.
4. Tinjau pengaturan layar, sentuh, audio, dan manajemen daya.

## Kompilasi dan Flash

```bash
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py flash
idf.py monitor
```

## Catatan

- Area sentuh tengah dipakai untuk percakapan, sedangkan sisi kiri dan kanan dipakai untuk volume.
- Perilaku sentuh dan ambang waktunya diatur di file `cst816x.h` dan `cst816x.cc`.
