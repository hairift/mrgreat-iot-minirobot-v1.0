# Panduan Gaya Kode

Panduan ini dipakai agar perubahan firmware Mr Great tetap aman, mudah diaudit, dan tidak merusak perilaku bawaan XiaoZhi.

## Prinsip

- Jangan menghapus fitur yang sudah berjalan kecuali ada keputusan eksplisit.
- Perubahan harus kecil, jelas, dan mudah dibandingkan dengan kode awal.
- Komentar kode memakai bahasa Indonesia.
- Nama fungsi, variabel, konstanta, dan API bawaan boleh tetap mengikuti gaya proyek asal.
- Hindari perubahan besar pada komponen vendor di `managed_components/`.

## C++

- Gunakan pola RAII dan mutex yang sudah ada di proyek.
- Jangan menahan lock saat operasi jaringan atau audio yang bisa lama.
- Untuk audio, hindari timeout tidak terbatas pada jalur output agar sistem tidak macet.
- Untuk servo, bedakan gerakan manual dan gerakan emosi.

## Python

- Script di `scripts/` harus aman dijalankan ulang.
- Output audit harus jelas dan memakai bahasa Indonesia bila ditampilkan ke pengguna.
- Jangan menulis file sementara ke folder source utama jika bisa memakai folder build atau temp.

## Dokumentasi

- Dokumentasi proyek utama memakai bahasa Indonesia.
- Jangan memakai ikon dekoratif atau emot di dokumentasi.
- Jika ada informasi wiring baru, perbarui `README.md`, `SERVO_README.md`, dan `docs/production-hardware-checklist.md`.

## Verifikasi Minimal

```powershell
idf.py build
python scripts\audit_campus_rag_server.py
```
