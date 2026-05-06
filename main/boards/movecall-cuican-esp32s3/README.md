# Movecall CuiCan ESP32-S3

## Menentukan target

```bash
idf.py set-target esp32s3
```

## Membuka konfigurasi

```bash
idf.py menuconfig
```

Pengaturan yang disarankan:

```text
Serial flasher config -> Flash size -> 8 MB
Partition Table -> Custom partition CSV file -> partitions/v2/8m.csv
Xiaozhi Assistant -> Board Type -> Movecall CuiCan
Component config -> Compiler options -> Optimization Level -> Optimize for size (-Os)
```

## Kompilasi

```bash
idf.py build
```
