# Taiji Pi S3

Mulai produksi sekitar Juli 2025, beberapa unit Taiji Pi S3 memakai mikrofon dan kaca layar yang berbeda. Jika nomor batch pada label produk lebih besar dari `2528`, pilih konfigurasi `I2S Type PDM`.

Papan ini juga mendukung konfigurasi audio dua kanal.

## Menentukan target

```bash
idf.py set-target esp32s3
```

## Membuka konfigurasi

```bash
idf.py menuconfig
```

Pilih papan dan tipe I2S berikut:

```text
Xiaozhi Assistant -> Board Type -> Taiji Pi S3
Xiaozhi Assistant -> TAIJIPAI_S3_CONFIG -> taiji-pi-S3 I2S Type -> I2S Type PDM
```

Jika ingin memakai audio dua kanal:

```text
Xiaozhi Assistant -> TAIJIPAI_S3_CONFIG -> Enabel use 2 slot
```

Pengaturan PSRAM yang disarankan:

```text
Component config -> ESP PSRAM -> SPI RAM config -> Try to allocate memories of WiFi and LWIP in SPIRAM firstly. If failed, allocate internal memory
```

## Kompilasi

```bash
idf.py build
```
