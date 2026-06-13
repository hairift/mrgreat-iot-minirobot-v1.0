# Protokol WebSocket

WebSocket adalah alternatif jalur komunikasi selain MQTT dan UDP. Pada konfigurasi Mr Great saat ini, jalur utama tetap mengikuti konfigurasi firmware yang dipilih lewat menuconfig.

## Kapan Dipakai

WebSocket berguna bila server AI atau lingkungan pengujian tidak memakai MQTT. Jalur ini dapat menyederhanakan pengujian karena satu koneksi membawa kontrol dan data percakapan.

## Catatan Implementasi

- Pastikan URL server benar.
- Pastikan sertifikat TLS cocok jika memakai `wss`.
- Jangan mengaktifkan dua jalur protokol utama secara bersamaan tanpa kebutuhan jelas.
- Jika audio patah pada WebSocket, cek ukuran buffer, kualitas jaringan, dan log task audio.

## Validasi

```powershell
idf.py menuconfig
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

Perangkat dianggap stabil jika bisa masuk state `idle`, mendengar kata bangun, menerima respons AI, dan kembali mendengarkan tanpa restart.
