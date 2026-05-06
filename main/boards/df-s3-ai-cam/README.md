# DF S3 AI Cam

Dokumentasi ini memakai format terbaru berbahasa Indonesia untuk board `df-s3-ai-cam`.

## Kompilasi Cepat

```bash
python scripts/release.py df-s3-ai-cam
```

## Kompilasi Manual

Buka konfigurasi proyek:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> df-s3-ai-cam
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=df-s3-ai-cam build
```
