# Panduan Gaya Kode

## Tujuan

Panduan ini dipakai untuk menjaga konsistensi format dan keterbacaan kode di seluruh proyek.

## Alat yang Dipakai

Proyek ini menggunakan `clang-format` dengan file konfigurasi `.clang-format` di akar repositori.

## Instalasi

### Windows

```powershell
winget install LLVM
```

### Linux

```bash
sudo apt install clang-format
```

### macOS

```bash
brew install clang-format
```

## Cara Pakai

### Format satu file

```bash
clang-format -i path/to/file.cc
```

### Format banyak file

```bash
find main -iname "*.h" -o -iname "*.cc" | xargs clang-format -i
```

### Cek tanpa mengubah file

```bash
clang-format --dry-run -Werror path/to/file.cc
```

## Aturan Umum

- gunakan indentasi 4 spasi
- batasi lebar baris secara wajar
- ikuti urutan include yang konsisten
- jangan merapikan alignment manual jika sudah ditangani formatter
- komentar kode dan dokumentasi proyek harus memakai bahasa Indonesia

## Integrasi Editor

### Visual Studio Code

- pasang ekstensi C atau C++
- aktifkan formatter `clang-format`
- aktifkan format saat simpan jika diperlukan

### CLion

- arahkan formatter ke `clang-format`
- gunakan file `.clang-format` dari repositori

## Catatan Penting

- format kode sebelum commit
- hindari perubahan format yang tidak perlu
- jika ada blok yang harus dikecualikan, gunakan penonaktifan `clang-format` secara lokal dan seperlunya
