# Labplus MPython V3

Dokumentasi ini memakai format terbaru berbahasa Indonesia untuk board `labplus-mpython-v3`.

## Kompilasi Cepat

```bash
python scripts/release.py labplus-mpython-v3
```

## Kompilasi Manual

Buka konfigurasi proyek:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> labplus-mpython-v3
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=labplus-mpython-v3 build
```
