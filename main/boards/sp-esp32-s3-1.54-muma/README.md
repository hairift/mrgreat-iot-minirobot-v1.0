# Spotpear ESP32-S3 LCD 1.54 MUMA

Papan ini adalah perangkat ESP32-S3 dengan layar ST7789 1,54 inci beresolusi 240x240. Perangkat mendukung asisten suara, tampilan cuaca dan jam, serta pemutaran video dari media yang didukung. Varian sentuh mendukung bangun dan interupsi lewat sentuhan.

Referensi produk:

- https://spotpear.cn/shop/ESP32-S3-AI-1.54-inch-LCD-Display-TouchScreen-N16R8-muma-DeepSeek/sp-esp32-s3-1.54-muma-W-Bat.html

## Menentukan target

```bash
idf.py set-target esp32s3
```

## Membuka konfigurasi

```bash
idf.py menuconfig
```

Pilih papan berikut:

```text
Xiaozhi Assistant -> Board Type -> Spotpear ESP32-S3-LCD-1.54-MUMA
```

## Kompilasi

```bash
idf.py build
```
