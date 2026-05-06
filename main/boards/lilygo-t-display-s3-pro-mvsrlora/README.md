# LILYGO T-Display-S3-Pro-MVSRLora

Dokumentasi ini memakai langkah kompilasi terbaru untuk papan `LILYGO T-Display-S3-Pro-MVSRLora`.

## Menentukan target

```bash
idf.py set-target esp32s3
```

## Membuka konfigurasi

```bash
idf.py menuconfig
```

Pilih salah satu tipe papan berikut:

```text
Xiaozhi Assistant -> Board Type -> LILYGO T-Display-S3-Pro-MVSRLora
```

atau:

```text
Xiaozhi Assistant -> Board Type -> LILYGO T-Display-S3-Pro-MVSRLora_NO_BATTERY
```

## Kompilasi

```bash
idf.py build
```

Referensi perangkat keras:

- https://github.com/Xinyuan-LilyGO/T-Display-S3-Pro
- https://github.com/Xinyuan-LilyGO/T-Display-S3-Pro-MVSRLora
