# Minsi K08 Dual

`minsi-k08-wifi` dan `minsi-k08-ml307` adalah varian perangkat dari Minsi Tech berbasis ESP32-S3 N16R8. Perangkat ini memakai penguat audio MAX98357 dan mikrofon omnidirectional INMP441, lalu dikemas ulang ke bentuk speaker K08 transparan bergaya punk dengan baterai besar.

Referensi produk:

- https://item.taobao.com/item.htm?id=889892765588

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
Xiaozhi Assistant -> Board Type -> Minsi K08 (DUAL)
```

## Kompilasi dan flash

```bash
idf.py build flash
```
