# Firmware Mr Great ESP32

## Gambaran Umum

Repositori ini berisi firmware asisten suara berbasis ESP32 yang telah disesuaikan untuk kebutuhan perangkat Mr Great. Proyek ini berjalan di atas arsitektur XiaoZhi ESP32, lalu diperkuat dengan:

- basis pengetahuan kampus UCIC yang diprioritaskan untuk seluruh pertanyaan kampus
- pencarian web untuk data dinamis atau waktu nyata
- kontrol servo dan gerakan robot yang lebih interaktif
- pengucapan bahasa Indonesia yang lebih konsisten untuk angka, biaya, alamat, dan narasi umum

## Pratinjau Perangkat

![Tampilan robot Mr Great](<docs/v0/mrgreatrobot (1).jpeg>)
![Tampilan robot Mr Great 2](<docs/v0/mrgreatrobot (2).jpeg>)
![Tampilan robot Mr Great 3](<docs/v0/mrgreatrobot (3).jpeg>)
![Tampilan robot Mr Great 4](<docs/v0/mrgreatrobot (4).jpeg>)
![Tampilan robot Mr Great 5](<docs/v0/mrgreatrobot (5).jpeg>)
![Tampilan robot Mr Great 6](<docs/v0/mrgreatrobot (6).jpeg>)
![Tampilan robot Mr Great 7](<docs/v0/mrgreatrobot (7).jpeg>)

Urutan prioritas jawaban yang digunakan proyek ini adalah:

1. `main/campus_data.cc` untuk semua pertanyaan kampus
2. `main/web_search.cc` untuk pertanyaan yang membutuhkan data terbaru
3. pengetahuan model untuk pertanyaan umum yang tidak membutuhkan data waktu nyata

## Fitur Utama

- kata bangun offline
- alur percakapan berbasis STT, LLM, dan TTS
- data kampus UCIC lokal di firmware
- pencarian web untuk tokoh publik, jabatan, dan fakta dinamis
- dukungan kontrol servo melalui suara dan MCP
- dukungan tampilan LCD atau OLED sesuai papan
- dukungan Wi-Fi, MQTT+UDP, atau WebSocket sesuai konfigurasi

## Struktur Direktori Penting

- `main/` sumber utama firmware
- `main/campus_data.cc` basis pengetahuan kampus lokal
- `main/web_search.cc` logika pencarian data dinamis
- `main/servo_controller.cc` kontrol gerakan servo
- `docs/` dokumentasi teknis proyek
- `scripts/` utilitas build dan alat bantu internal

## Kompilasi dan Unggah

Gunakan lingkungan ESP-IDF yang sesuai dengan proyek ini. Perintah dasar:

```powershell
cd E:\MrGreat-IOT-ESP32\MrGreat-esp32-main
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

Jika port serial bermasalah saat flash, gunakan baud rate lebih rendah:

```powershell
idf.py -p COM3 -b 115200 flash
```

## Catatan Implementasi Kampus

- Pertanyaan kampus tidak harus menyebut kata `UCIC` secara eksplisit.
- Jalur pencarian kampus memakai indeks retrieval yang dibangun dari `CAMPUS_DB`.
- Query dosen per mata kuliah diaudit otomatis agar nama pengampu tidak mudah tertukar.
- Typo ringan dan variasi penulisan umum ditangani di jalur pencarian kampus.

## Dokumentasi Terkait

- [Panduan servo](SERVO_README.md)
- [Konfigurasi BluFi](docs/blufi.md)
- [Panduan gaya kode](docs/code_style.md)
- [Panduan papan kustom](docs/custom-board.md)
- [Protokol MCP](docs/mcp-protocol.md)
- [Penggunaan MCP](docs/mcp-usage.md)
- [Protokol MQTT + UDP](docs/mqtt-udp.md)
- [Daftar validasi perangkat keras](docs/production-hardware-checklist.md)
- [Protokol WebSocket](docs/websocket.md)

## Catatan Penting

- File di `build/` adalah artefak hasil kompilasi dan dapat berubah setiap build.
- File di `managed_components/` adalah dependensi terkelola dan sebaiknya tidak diubah tanpa alasan kuat.
- Sebelum rilis firmware, jalankan validasi kampus, validasi audio, validasi servo, dan validasi konektivitas.

## Lisensi dan Penggunaan

Perhatikan lisensi proyek induk dan lisensi dependensi yang disertakan di dalam repositori sebelum digunakan untuk distribusi, modifikasi, atau kebutuhan komersial.
