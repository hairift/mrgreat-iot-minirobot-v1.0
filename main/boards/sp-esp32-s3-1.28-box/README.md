# Spotpear ESP32-S3 1.28 Box

Papan ini memakai layar bundar 1,28 inci, mendukung sentuhan, dan mendukung pengisian daya baterai.

## Referensi Perangkat

- Tautan produk 1: `https://spotpear.cn/shop/ESP32-S3-N16R8-AI-DeepSeek-XiaoZhi-XiaGe-Qwen-DouBao-1.28-inch-LCD.html`
- Tautan produk 2: `https://spotpear.cn/shop/ESP32-S3-N16R8-AI-DeepSeek-XiaoZhi-XiaGe-Qwen-DouBao-1.28-inch-Round-LCD-BOX-TouchScreen.html`

## Kompilasi Cepat

```bash
python scripts/release.py sp-esp32-s3-1.28-box
```

## Kompilasi Manual

Atur target chip:

```bash
idf.py set-target esp32s3
```

Buka konfigurasi:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> sp-esp32-s3-1.28-box
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=sp-esp32-s3-1.28-box build
```
