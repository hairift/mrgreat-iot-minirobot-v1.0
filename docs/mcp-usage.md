# Penggunaan MCP

Dokumen ini merangkum cara memakai alat MCP yang tersedia di firmware Mr Great.

## Data Kampus

Gunakan `self.campus.query` untuk pertanyaan tentang UCIC, dosen, jurusan, biaya, akreditasi, kurikulum, fasilitas, dan kontak.

Contoh:

```json
{
  "keyword": "siapa rektor UCIC"
}
```

## Pencarian Web

Gunakan `self.web.search` untuk pertanyaan yang membutuhkan data terbaru, misalnya pejabat aktif, berita, jadwal, harga, atau informasi yang mudah berubah.

Contoh:

```json
{
  "query": "siapa gubernur Jawa Barat saat ini"
}
```

## Servo

Gunakan `self.servo.power` untuk daya servo dan `self.servo.action` untuk aksi.

Contoh:

```json
{
  "action": "wave_right"
}
```

## Layar dan Audio

Volume speaker dapat diatur melalui `self.audio_speaker.set_volume`. Tema layar dapat diubah melalui `self.screen.set_theme` jika board mendukung.

## Prinsip Aman

- Jangan memanggil servo terus-menerus tanpa jeda.
- Jangan memakai web search untuk data kampus yang sudah ada di `campus_data.cc`.
- Jika hasil alat tidak sesuai, minta klarifikasi atau jawab bahwa data belum cukup.
