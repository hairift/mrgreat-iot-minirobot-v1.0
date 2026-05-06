# Protokol MQTT dan UDP

Dokumen ini menjelaskan pola komunikasi gabungan MQTT dan UDP yang dipakai proyek.

## Tujuan

- MQTT dipakai untuk pesan kendali, status, dan sinkronisasi sesi
- UDP dipakai untuk aliran audio yang membutuhkan latensi rendah

## Alur Ringkas

1. Perangkat tersambung ke broker MQTT.
2. Perangkat mengirim pesan `hello` untuk meminta sesi audio.
3. Peladen membalas dengan informasi sesi UDP.
4. Perangkat membuka jalur UDP sesuai informasi tersebut.
5. Audio dikirim melalui UDP, sedangkan kendali sesi tetap lewat MQTT.

## Data Kendali melalui MQTT

Contoh pesan yang umum dipakai:

- `hello`
- `listen`
- `abort`
- `mcp`
- `goodbye`

## Data Audio melalui UDP

Saluran UDP dipakai untuk frame audio yang menuntut jeda serendah mungkin. Jalur ini dapat dilengkapi enkripsi, nomor urut paket, dan informasi sesi agar dua arah audio tetap sinkron.

## Hal yang Perlu Dicek

- perangkat dapat menyambung ulang ke MQTT secara otomatis
- sesi UDP dapat dibuka setelah `hello` berhasil
- audio dua arah tetap sinkron selama percakapan
- putus jaringan tidak membuat status aplikasi macet

## Kapan Dipakai

Gunakan pola ini jika penempatan sistem membutuhkan pemisahan yang tegas antara saluran kendali dan saluran audio.
