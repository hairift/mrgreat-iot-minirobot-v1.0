# Panduan Papan Kustom

Dokumen ini menjelaskan langkah dasar untuk menambahkan papan baru ke proyek.

## Prinsip Umum

- jangan menimpa papan bawaan jika perangkat Anda berbeda
- buat direktori papan baru dengan nama unik
- pastikan nama kompilasi dan identitas papan tidak bentrok dengan papan lain

## Struktur Minimal

Setiap papan umumnya memiliki file berikut:

- `xxx_board.cc` untuk inisialisasi utama
- `config.h` untuk pin dan parameter papan
- `config.json` untuk target chip dan opsi kompilasi
- `README.md` jika papan memerlukan penjelasan tambahan

## Langkah Penambahan Papan

### 1. Buat direktori baru

Contoh:

```bash
mkdir main/boards/my-custom-board
```

### 2. Buat `config.h`

Isi file ini dengan:

- konfigurasi I2S audio
- konfigurasi I2C codec
- pin tombol
- pin layar
- parameter lampu latar atau LED jika ada

### 3. Buat `config.json`

Definisikan:

- target chip
- nama kompilasi
- tambahan `sdkconfig_append` jika perlu

Contoh nilai yang umum:

- ukuran memori flash
- tabel partisi
- konfigurasi bahasa
- fitur audio atau kata bangun

### 4. Buat file papan utama

File papan utama bertugas:

- inisialisasi I2C atau SPI
- inisialisasi codec audio
- inisialisasi layar
- inisialisasi tombol
- pendaftaran papan ke sistem

### 5. Kompilasi dan uji

Jalankan:

```powershell
cd E:\MrGreat-IOT-ESP32\MrGreat-esp32-main
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

## Hal yang Wajib Dicek

- boot normal
- audio masukan dan keluaran normal
- koneksi jaringan normal
- layar normal
- konsumsi daya sesuai kebutuhan papan
- fitur khusus papan tidak bentrok dengan papan lain
