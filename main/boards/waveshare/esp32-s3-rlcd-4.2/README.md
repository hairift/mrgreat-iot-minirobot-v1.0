# Waveshare ESP32-S3 RLCD 4.2

Referensi produk:

- https://www.waveshare.net/shop/ESP32-S3-RLCD-4.2.htm

## Menentukan target

```bash
idf.py set-target esp32s3
```

## Membuka konfigurasi

```bash
idf.py menuconfig
```

Pilih tipe board `esp32-s3-rlcd-4.2` pada menu `Xiaozhi Assistant -> Board Type`.

## Kompilasi

```bash
idf.py build
```

## Penulisan Firmware dan Monitor

```bash
idf.py flash monitor
```
