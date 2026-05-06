# ESP32 CGC 144

Dokumentasi ini memakai format terbaru berbahasa Indonesia untuk board `esp32-cgc-144`.

## Kompilasi Cepat

```bash
python scripts/release.py esp32-cgc-144
```

## Kompilasi Manual

Buka konfigurasi proyek:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> esp32-cgc-144
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=esp32-cgc-144 build
```
