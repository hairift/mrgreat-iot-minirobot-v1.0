# Protokol MQTT dan UDP

Firmware Mr Great memakai MQTT untuk kontrol sesi dan UDP untuk pengiriman audio real-time. Keduanya harus stabil agar suara AI tidak patah atau hilang.

## Alur Koneksi

1. Perangkat tersambung ke Wi-Fi.
2. Firmware melakukan aktivasi.
3. Firmware membuka koneksi MQTT.
4. Saat percakapan aktif, audio dikirim dan diterima melalui jalur real-time.

## Gejala Jaringan Tidak Stabil

- Log menampilkan paket audio dengan sequence tidak sesuai.
- Suara TTS terdengar patah.
- Respons AI berhenti di tengah.
- Perangkat kembali ke state koneksi jika jaringan benar-benar putus.

## Mitigasi Firmware

- Antrean audio dibuat lebih longgar agar jitter jaringan ringan tidak langsung memutus playback.
- Timeout output audio dibatasi agar task tidak macet selamanya.
- Mode hemat daya Wi-Fi diturunkan saat percakapan aktif.

## Validasi

```powershell
idf.py -p COM3 monitor
```

Perhatikan log `MQTT`, `AudioService`, `StateMachine`, dan `SystemInfo`. Jika sequence audio sering salah, perbaiki kualitas sinyal Wi-Fi terlebih dahulu.
