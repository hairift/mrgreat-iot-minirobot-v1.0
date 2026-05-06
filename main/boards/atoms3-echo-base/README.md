# AtomS3 Echo Base

Dokumentasi ini memakai format terbaru berbahasa Indonesia untuk board `atoms3-echo-base`.

## Kompilasi Cepat

```bash
python scripts/release.py atoms3-echo-base
```

## Kompilasi Manual

Buka konfigurasi proyek:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> atoms3-echo-base
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=atoms3-echo-base build
```
