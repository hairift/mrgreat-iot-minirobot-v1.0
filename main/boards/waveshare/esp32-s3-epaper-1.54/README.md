# Waveshare ESP32-S3 ePaper 1.54

Referensi produk:

- https://www.waveshare.net/shop/ESP32-S3-ePaper-1.54.htm

## Menentukan target

```bash
idf.py set-target esp32s3
```

## Membuka konfigurasi

```bash
idf.py menuconfig
```

Pilih tipe board `esp32-s3-epaper-1.54` pada menu `Xiaozhi Assistant -> Board Type`.

## Kompilasi

```bash
idf.py build
```

## Penulisan Firmware dan Monitor

```bash
idf.py flash monitor
```
