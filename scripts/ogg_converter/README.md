# Konverter OGG untuk Mr Great

Skrip ini dipakai untuk mengonversi audio ke format OGG yang cocok dipakai oleh
proyek Mr Great. Alat ini juga mendukung konversi dua arah antara OGG dan
format audio umum serta penyesuaian amplitudo.

Implementasinya memakai pustaka `ffmpeg-python`, sehingga `ffmpeg` harus sudah
tersedia di sistem.

## Menyiapkan Lingkungan Virtual

```bash
python -m venv venv
source venv/bin/activate
```

Untuk Windows:

```powershell
venv\Scripts\activate
```

## Memasang FFmpeg

Unduh FFmpeg dari:
https://ffmpeg.org/download.html

Tambahkan executable FFmpeg ke `PATH`, atau letakkan di direktori yang sama
dengan skrip ini.

## Memasang Dependensi Python

```bash
pip install ffmpeg-python
```

## Menjalankan Skrip

```bash
python ogg_covertor.py
```
