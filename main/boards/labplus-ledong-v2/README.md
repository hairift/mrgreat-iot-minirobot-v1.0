# Labplus Ledong V2

Dokumentasi ini memakai format terbaru berbahasa Indonesia untuk board `labplus-ledong-v2`.

## Kompilasi Cepat

```bash
python scripts/release.py labplus-ledong-v2
```

## Kompilasi Manual

Buka konfigurasi proyek:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> labplus-ledong-v2
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=labplus-ledong-v2 build
```
