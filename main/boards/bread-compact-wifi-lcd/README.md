# Bread Compact WiFi LCD

Papan ini adalah varian bread-compact dengan koneksi Wi-Fi dan layar LCD.

## Kompilasi Cepat

```bash
python scripts/release.py bread-compact-wifi-lcd
```

## Kompilasi Manual

Atur target chip:

```bash
idf.py set-target esp32s3
```

Buka konfigurasi:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> bread-compact-wifi-lcd
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=bread-compact-wifi-lcd build
```
