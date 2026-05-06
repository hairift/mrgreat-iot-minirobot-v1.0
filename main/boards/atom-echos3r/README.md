# Atom Echo S3R

Dokumentasi ini memakai format terbaru berbahasa Indonesia untuk board `atom-echos3r`.

## Kompilasi Cepat

```bash
python scripts/release.py atom-echos3r
```

## Kompilasi Manual

Buka konfigurasi proyek:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> atom-echos3r
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=atom-echos3r build
```
