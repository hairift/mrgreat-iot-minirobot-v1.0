# ESP Hi

Dokumentasi ini memakai format terbaru berbahasa Indonesia untuk board `esp-hi`.

## Kompilasi Cepat

```bash
python scripts/release.py esp-hi
```

## Kompilasi Manual

Buka konfigurasi proyek:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> esp-hi
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=esp-hi build
```
