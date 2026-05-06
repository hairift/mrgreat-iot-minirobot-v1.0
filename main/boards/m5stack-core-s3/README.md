# M5Stack Core S3

Dokumentasi ini memakai format terbaru berbahasa Indonesia untuk board `m5stack-core-s3`.

## Kompilasi Cepat

```bash
python scripts/release.py m5stack-core-s3
```

## Kompilasi Manual

Buka konfigurasi proyek:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> m5stack-core-s3
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=m5stack-core-s3 build
```
