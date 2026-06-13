# Konfigurasi BluFi

BluFi dipakai untuk mengirim konfigurasi Wi-Fi ke perangkat saat belum ada SSID yang tersimpan. Pada firmware Mr Great, jalur utama tetap memakai Wi-Fi station, sedangkan BluFi menjadi opsi provisioning jika dibutuhkan.

## Alur Dasar

1. Perangkat menyala dan memeriksa konfigurasi Wi-Fi tersimpan.
2. Jika konfigurasi belum ada atau gagal tersambung, mode provisioning dapat dijalankan.
3. Pengguna mengirim SSID dan kata sandi melalui aplikasi atau alat yang mendukung BluFi.
4. Perangkat menyimpan konfigurasi lalu mencoba koneksi ulang.

## Catatan Keamanan

- Jangan menulis SSID dan kata sandi secara permanen di source code.
- Gunakan koneksi provisioning hanya saat dibutuhkan.
- Setelah Wi-Fi tersambung, pastikan perangkat kembali ke mode operasi normal.

## Validasi

```powershell
cd E:\MrGreat-IOT-ESP32\MrGreat-esp32-main
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

Perhatikan log Wi-Fi. Perangkat harus bisa mendapatkan alamat IP dan masuk ke state `idle` tanpa reboot loop.
