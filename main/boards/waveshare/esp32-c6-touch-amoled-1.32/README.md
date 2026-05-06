# Waveshare ESP32-C6 Touch AMOLED 1.32

Referensi produk:

- https://www.waveshare.net/shop/ESP32-C6-Touch-AMOLED-1.32.htm

## Menentukan target

```bash
idf.py set-target esp32c6
```

## Membuka konfigurasi

```bash
idf.py menuconfig
```

Pilih tipe board `esp32-c6-touch-amoled-1.32` pada menu `Xiaozhi Assistant -> Board Type`.

## Kompilasi

```bash
idf.py build
```

## Penulisan Firmware dan Monitor

```bash
idf.py flash monitor
```
