# Tabel Partisi Versi 2

Versi ini membawa peningkatan besar dibanding `v1` dengan menambahkan partisi `assets` untuk memuat konten dari jaringan dan menata ulang partisi agar lebih efisien pada berbagai ukuran flash.

## Perubahan utama dari v1

1. Partisi `assets` baru untuk konten yang dapat diunduh dari jaringan.
2. Partisi `model` lama digantikan oleh partisi `assets` yang lebih besar.
3. Ukuran partisi aplikasi dioptimalkan agar ada ruang untuk aset dinamis.
4. Konten dapat diperbarui tanpa flash ulang firmware utama.

## Isi partisi assets

Partisi `assets` dipakai untuk menyimpan:

- model kata bangun yang dapat diganti
- tema lengkap, termasuk font, ikon, audio, gambar latar, dan emoji
- berkas konfigurasi bahasa
- konten tambahan lain yang diunduh lewat jaringan

## Perbandingan tata letak

### Tata letak v1 untuk 16 MB

- `nvs`: 16 KB
- `otadata`: 8 KB
- `phy_init`: 4 KB
- `model`: 960 KB
- `ota_0`: 6 MB
- `ota_1`: 6 MB

### Tata letak v2 untuk 16 MB

- `nvs`: 16 KB
- `otadata`: 8 KB
- `phy_init`: 4 KB
- `ota_0`: 4 MB
- `ota_1`: 4 MB
- `assets`: 8 MB

## Konfigurasi yang tersedia

### Perangkat flash 8 MB (`8m.csv`)

- `nvs`: 16 KB
- `otadata`: 8 KB
- `phy_init`: 4 KB
- `ota_0`: 3 MB
- `ota_1`: 3 MB
- `assets`: 2 MB

### Perangkat flash 16 MB (`16m.csv`)

- `nvs`: 16 KB
- `otadata`: 8 KB
- `phy_init`: 4 KB
- `ota_0`: 4 MB
- `ota_1`: 4 MB
- `assets`: 8 MB

### Perangkat flash 16 MB (`16m_c3.csv`) untuk ESP32-C3

- `nvs`: 16 KB
- `otadata`: 8 KB
- `phy_init`: 4 KB
- `ota_0`: 4 MB
- `ota_1`: 4 MB
- `assets`: 4 MB

### Perangkat flash 32 MB (`32m.csv`)

- `nvsfactory`: 200 KB
- `nvs`: 840 KB
- `otadata`: 8 KB
- `phy_init`: 4 KB
- `ota_0`: 4 MB
- `ota_1`: 4 MB
- `assets`: 16 MB

## Manfaat

1. Aset dapat diperbarui secara dinamis tanpa flash ulang firmware utama.
2. Pengguna bisa mengganti model kata bangun, tema, dan paket bahasa dengan lebih fleksibel.
3. Penggunaan flash lebih efisien karena ruang aset dipisahkan dari ruang aplikasi.
4. Proses pembaruan lewat jaringan menjadi lebih aman dan terarah.

## Detail teknis

- Partisi `assets` memakai subtype `spiffs`.
- Aset dipetakan ke memori agar akses saat runtime lebih efisien.
- Validasi checksum dipakai untuk menjaga integritas data.
- Unduhan aset dapat dilakukan bertahap dengan pelacakan progres.
- Jika pembaruan gagal, sistem dapat kembali memakai aset bawaan.

## Migrasi dari v1

Saat berpindah dari `v1` ke `v2`:

1. Cadangkan data penting pada partisi lama jika masih diperlukan.
2. Flash tabel partisi `v2` yang sesuai dengan ukuran flash perangkat.
3. Biarkan perangkat mengunduh aset yang dibutuhkan saat boot pertama.
4. Verifikasi kembali semua fungsi utama setelah migrasi selesai.
