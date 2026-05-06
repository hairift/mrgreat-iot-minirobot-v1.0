# XMini C3 4G

Dokumentasi ini memakai format terbaru berbahasa Indonesia untuk board `xmini-c3-4g`.

## Kompilasi Cepat

```bash
python scripts/release.py xmini-c3-4g
```

## Kompilasi Manual

Buka konfigurasi proyek:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> xmini-c3-4g
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=xmini-c3-4g build
```
