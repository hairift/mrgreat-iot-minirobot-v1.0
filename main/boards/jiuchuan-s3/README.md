# Jiuchuan S3

Dokumentasi ini memakai format terbaru berbahasa Indonesia untuk board `jiuchuan-s3`.

## Kompilasi Cepat

```bash
python scripts/release.py jiuchuan-s3
```

## Kompilasi Manual

Buka konfigurasi proyek:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> jiuchuan-s3
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=jiuchuan-s3 build
```
