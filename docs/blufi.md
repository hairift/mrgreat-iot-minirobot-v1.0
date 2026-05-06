# Panduan Konfigurasi BluFi

Dokumen ini menjelaskan cara mengaktifkan dan menggunakan BluFi untuk provisioning Wi-Fi pada firmware ini.

## Tujuan

BluFi dipakai ketika perangkat perlu menerima SSID dan kata sandi Wi-Fi melalui BLE. Jalur ini cocok untuk kondisi awal saat perangkat belum memiliki jaringan tersimpan.

## Prasyarat

- papan dan firmware mendukung BLE
- NVS aktif dan loop event bawaan sudah berjalan
- hanya satu stack Bluetooth yang diaktifkan sesuai konfigurasi proyek

## Pengaturan Menuconfig

Aktifkan metode konfigurasi Wi-Fi berbasis BluFi sesuai opsi proyek yang tersedia. Jangan aktifkan provisioning hotspot pada saat yang sama jika ingin memaksa jalur BluFi.

## Alur Kerja

1. Perangkat masuk ke mode provisioning.
2. Aplikasi ponsel terhubung ke perangkat melalui BluFi.
3. SSID dan kata sandi dikirim ke perangkat.
4. Perangkat menyimpan kredensial dan mencoba tersambung ke Wi-Fi.
5. Status koneksi dikirim kembali ke aplikasi.

## Langkah Penggunaan

1. Kompilasi lalu tulis firmware dengan BluFi aktif.
2. Nyalakan perangkat dari kondisi tanpa Wi-Fi tersimpan.
3. Buka aplikasi klien BluFi di ponsel.
4. Pilih perangkat, lalu kirim SSID dan kata sandi.
5. Pastikan perangkat berhasil tersambung dan menyimpan konfigurasi.

## Hal yang Perlu Diperhatikan

- jangan mengaktifkan dua metode provisioning sekaligus bila tidak dibutuhkan
- jika pengujian diulang berkali-kali, hapus kredensial Wi-Fi lama agar hasil tidak rancu
- gunakan aplikasi klien yang mengikuti format paket BluFi resmi

## Verifikasi

- perangkat menerima kredensial
- perangkat dapat tersambung ke access point
- perangkat dapat menyambung ulang otomatis setelah restart
