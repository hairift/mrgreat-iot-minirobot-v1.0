# ESP Spot

Dokumentasi ini memakai format terbaru berbahasa Indonesia untuk board `esp-spot`.

## Kompilasi Cepat

```bash
python scripts/release.py esp-spot
```

## Kompilasi Manual

Buka konfigurasi proyek:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> esp-spot
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=esp-spot build
```
