# Pengujian Gelombang Suara

Direktori ini berisi alat GUI untuk memantau audio PCM yang dikirim balik oleh
perangkat melalui UDP, lalu menampilkan hasilnya pada domain waktu dan domain
frekuensi. Hasil tangkapan juga bisa disimpan untuk menganalisis sebaran noise
dan mengecek akurasi transmisi data suara berbasis ASCII.

## Prasyarat

- firmware harus dibangun dengan `USE_AUDIO_DEBUGGER`
- `AUDIO_DEBUG_UDP_SERVER` harus diarahkan ke alamat komputer lokal

Uji suara demodulasi dapat dijalankan melalui `sonic_wifi_config.html` atau
melalui halaman distribusi suara yang digunakan proyek.

## Ringkasan Hasil Uji

Simbol yang dipakai:

- `OK` berarti sinyal PCM mentah dari I2S DIN sudah bisa didekode dengan baik
- `PERLU TUNING` berarti perlu peredaman noise atau langkah tambahan agar hasil stabil
- `GAGAL` berarti hasil tetap buruk walau sudah diberi perlakuan tambahan

Beberapa ADC membutuhkan penyesuaian noise reduction yang lebih rinci saat
konfigurasi I2C. Karena tiap perangkat tidak selalu sama, hasil di bawah ini
mengacu pada konfigurasi board yang tersedia di repositori.

| Perangkat | ADC | Mikrofon | Hasil | Catatan |
|---|---|---|---|---|
| bread-compact | INMP441 | MEMS mic terintegrasi | OK | |
| lichuang-dev | ES7210 | | PERLU TUNING | Saat pengujian perlu menonaktifkan `INPUT_REFERENCE` |
| kevin-box-2 | ES7210 | | PERLU TUNING | Saat pengujian perlu menonaktifkan `INPUT_REFERENCE` |
| m5stack-core-s3 | ES7210 | | PERLU TUNING | Saat pengujian perlu menonaktifkan `INPUT_REFERENCE` |
| xmini-c3 | ES8311 | | PERLU TUNING | Perlu peredaman noise |
| atoms3r-echo-base | ES8311 | | PERLU TUNING | Perlu peredaman noise |
| atk-dnesp32s3-box0 | ES8311 | | GAGAL | Bisa menerima dan mendekode, tetapi paket banyak hilang |
| movecall-moji-esp32s3 | ES8311 | | GAGAL | Bisa menerima dan mendekode, tetapi paket banyak hilang |
