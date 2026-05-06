# LilyGO T Circle S3

Dokumentasi ini memakai format terbaru berbahasa Indonesia untuk board `lilygo-t-circle-s3`.

## Kompilasi Cepat

```bash
python scripts/release.py lilygo-t-circle-s3
```

## Kompilasi Manual

Buka konfigurasi proyek:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> lilygo-t-circle-s3
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=lilygo-t-circle-s3 build
```
