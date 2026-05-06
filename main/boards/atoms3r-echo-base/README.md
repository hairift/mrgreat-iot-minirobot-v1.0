# AtomS3R Echo Base

Dokumentasi ini memakai format terbaru berbahasa Indonesia untuk board `atoms3r-echo-base`.

## Kompilasi Cepat

```bash
python scripts/release.py atoms3r-echo-base
```

## Kompilasi Manual

Buka konfigurasi proyek:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> atoms3r-echo-base
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=atoms3r-echo-base build
```
