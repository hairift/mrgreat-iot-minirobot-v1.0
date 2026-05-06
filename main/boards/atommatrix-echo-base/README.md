# AtomMatrix Echo Base

Dokumentasi ini memakai format terbaru berbahasa Indonesia untuk board `atommatrix-echo-base`.

## Kompilasi Cepat

```bash
python scripts/release.py atommatrix-echo-base
```

## Kompilasi Manual

Buka konfigurasi proyek:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> atommatrix-echo-base
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=atommatrix-echo-base build
```
