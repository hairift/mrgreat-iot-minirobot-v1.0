# ESP SensAirShuttle

Dokumentasi ini memakai format terbaru berbahasa Indonesia untuk board `esp-sensairshuttle`.

## Kompilasi Cepat

```bash
python scripts/release.py esp-sensairshuttle
```

## Kompilasi Manual

Buka konfigurasi proyek:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> esp-sensairshuttle
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=esp-sensairshuttle build
```
