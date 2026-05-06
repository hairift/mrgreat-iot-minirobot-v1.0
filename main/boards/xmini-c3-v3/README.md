# XMini C3 V3

Dokumentasi ini memakai format terbaru berbahasa Indonesia untuk board `xmini-c3-v3`.

## Kompilasi Cepat

```bash
python scripts/release.py xmini-c3-v3
```

## Kompilasi Manual

Buka konfigurasi proyek:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> xmini-c3-v3
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=xmini-c3-v3 build
```
