# Daftar Validasi Perangkat Keras Produksi

Dokumen ini dipakai sebagai daftar pemeriksaan sebelum firmware dinyatakan siap untuk produksi.

## 1. Kompilasi dan artefak

- `xiaozhi.bin`, `xiaozhi.elf`, dan `generated_assets.bin` berhasil dibuat dari source terbaru
- parameter memori flash sesuai dengan papan target
- offset partisi sesuai skema proyek

## 2. Penulisan firmware dan boot

- firmware berhasil ditulis ke perangkat tanpa kesalahan serial
- boot awal tidak mengalami panic, abort, atau reboot loop
- log inisialisasi audio, jaringan, dan board berjalan normal

## 3. Jaringan

- provisioning berhasil dari kondisi awal
- sambung ulang otomatis berhasil setelah router dihidupkan ulang
- siklus mati-hidup daya berulang tidak merusak konfigurasi jaringan

## 4. Audio

- kata bangun terdeteksi secara konsisten
- audio masukan dan keluaran berjalan normal
- TTS panjang tidak terputus
- interupsi percakapan tidak merusak status aplikasi

## 5. Data kampus

- pertanyaan kampus memprioritaskan `campus_data.cc`
- jawaban dosen, biaya, kurikulum, profil, akreditasi, dan fasilitas sesuai dataset
- AI tidak mengarang ketika data kampus tersedia

## 6. Pencarian web

- pertanyaan yang memerlukan data terbaru memakai `web_search.cc`
- jawaban tokoh publik dan jabatan mengarah ke hasil yang relevan
- saat pencarian gagal, AI menjawab jujur tanpa halusinasi

## 7. Servo dan gerakan

- servo dapat aktif dan nonaktif sesuai perintah
- gerakan kepala tetap kiri-kanan
- gerakan emosi tidak bentrok dengan perintah eksplisit pengguna
- kata bangun dan gerakan bicara berjalan normal

## 8. Keandalan

- perangkat stabil untuk percakapan panjang
- tidak ada kebocoran memori yang terlihat
- tidak ada task audio atau protokol yang macet

## 9. Keputusan rilis

Firmware layak produksi bila seluruh poin di atas lulus dan tidak ditemukan regresi pada fungsi utama proyek.
