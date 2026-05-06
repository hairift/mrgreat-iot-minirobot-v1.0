# ESP Vocat

Dokumentasi ini memakai format terbaru berbahasa Indonesia untuk board `esp-vocat`.

## Kompilasi Cepat

```bash
python scripts/release.py esp-vocat
```

## Kompilasi Manual

Buka konfigurasi proyek:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> esp-vocat
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=esp-vocat build
```
