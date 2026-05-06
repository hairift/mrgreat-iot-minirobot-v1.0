# Surfer C3 1.14 TFT

Papan ini merupakan adaptasi dari papan Surfer ESP32-C3 dengan perubahan pada pin lampu latar TFT dan tambahan pemantauan level baterai lewat ADC.

## Catatan Penting

- Ukuran flash papan ini adalah 16 MB.
- Saat kompilasi, gunakan tabel partisi bawaan yang sesuai dengan konfigurasi papan.

## Kompilasi Cepat

```bash
python scripts/release.py surfer-c3-1.14tft
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
Xiaozhi Assistant -> Board Type -> surfer-c3-1.14tft
```

Kompilasi firmware:

```bash
idf.py -DBOARD_NAME=surfer-c3-1.14tft build
```
