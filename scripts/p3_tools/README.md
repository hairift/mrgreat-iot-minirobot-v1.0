# Alat Konversi dan Pemutaran Audio P3

Direktori ini berisi utilitas Python untuk mengolah berkas audio berformat P3.

## `convert_audio_to_p3.py`

Mengubah berkas audio umum menjadi format P3 dengan struktur header 4 byte dan
paket data Opus. Skrip ini juga mendukung normalisasi loudness.

### Cara Pakai

```bash
python convert_audio_to_p3.py <audio_masuk> <berkas_p3_keluar> [-l LUFS] [-d]
```

- `-l` menentukan target loudness, bawaan `-16 LUFS`
- `-d` menonaktifkan normalisasi loudness

Sebaiknya gunakan `-d` jika audio sangat pendek, loudness sudah disesuaikan
sebelumnya, atau sumbernya sudah berasal dari TTS bawaan proyek.

## `play_p3.py`

Memutar berkas audio P3.

### Fitur

- mendekode dan memutar audio P3
- menerapkan efek fade out saat playback berakhir atau dihentikan
- menerima path berkas dari argumen command line

### Cara Pakai

```bash
python play_p3.py <path_berkas_p3>
```

## `convert_p3_to_audio.py`

Mengubah berkas P3 kembali menjadi format audio umum.

### Cara Pakai

```bash
python convert_p3_to_audio.py <berkas_p3_masuk> <berkas_audio_keluar>
```

Berkas audio keluaran harus memiliki ekstensi yang sesuai, misalnya `.wav`.

## `batch_convert_gui.py`

Antarmuka grafis untuk konversi massal dari audio ke P3 maupun dari P3 ke audio.

### Cara Pakai

```bash
python batch_convert_gui.py
```

## Dependensi

```bash
pip install librosa opuslib numpy tqdm sounddevice pyloudnorm soundfile
```

Atau gunakan:

```bash
pip install -r requirements.txt
```

## Ringkasan Format P3

- setiap frame audio terdiri dari header 4 byte dan satu paket data Opus
- format header: `[1 byte tipe, 1 byte cadangan, 2 byte panjang]`
- sample rate tetap `16000 Hz`
- mono channel
- durasi per frame `60 ms`
