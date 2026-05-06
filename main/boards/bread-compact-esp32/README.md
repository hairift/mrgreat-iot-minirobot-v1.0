# Bread Compact ESP32

Papan ini menggunakan ESP32 DevKit dengan layar OLED kecil dan kontrol tombol lokal.

## Kompilasi Cepat

```bash
python scripts/release.py bread-compact-esp32
```

## Kompilasi Manual

Atur target chip:

```bash
idf.py set-target esp32
```

Buka konfigurasi:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> bread-compact-esp32
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=bread-compact-esp32 build
```

Tulis firmware ke perangkat:

```bash
idf.py -p COM3 flash
```
