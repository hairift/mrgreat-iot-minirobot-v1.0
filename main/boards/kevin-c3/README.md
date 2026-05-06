# Kevin C3

Papan ini memakai ESP32-C3 dan terhubung dengan strip LED yang bisa dikendalikan dari perangkat.

## Kompilasi Cepat

```bash
python scripts/release.py kevin-c3
```

## Kompilasi Manual

Atur target chip:

```bash
idf.py set-target esp32c3
```

Buka konfigurasi:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> kevin-c3
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=kevin-c3 build
```
