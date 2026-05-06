# M5Stack Cardputer Adv

Dokumentasi ini memakai format terbaru berbahasa Indonesia untuk board `m5stack-cardputer-adv`.

## Kompilasi Cepat

```bash
python scripts/release.py m5stack-cardputer-adv
```

## Kompilasi Manual

Buka konfigurasi proyek:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> m5stack-cardputer-adv
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=m5stack-cardputer-adv build
```
