# M5Stack Tab5

Dokumentasi ini memakai format terbaru berbahasa Indonesia untuk board `m5stack-tab5`.

## Kompilasi Cepat

```bash
python scripts/release.py m5stack-tab5
```

## Kompilasi Manual

Buka konfigurasi proyek:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> m5stack-tab5
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=m5stack-tab5 build
```
