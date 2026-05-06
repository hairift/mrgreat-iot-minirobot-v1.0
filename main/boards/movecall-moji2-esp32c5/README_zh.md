# Movecall Moji2 ESP32-C5

Dokumentasi ini menjelaskan cara kompilasi firmware untuk perangkat `Movecall Moji2`.

## Prasyarat

- ESP-IDF `v5.5`
- Target chip `ESP32-C5`

Referensi perangkat keras:

- https://oshwhub.com/movecall/moji2

## Menentukan target

```bash
idf.py set-target esp32c5
```

## Membuka konfigurasi

```bash
idf.py menuconfig
```

Pilih tipe board `Movecall Moji2` pada menu `Xiaozhi Assistant -> Board Type`.

## Kompilasi

```bash
idf.py build
```

## Perintah tambahan

```bash
idf.py fullclean
idf.py flash
idf.py monitor
```
