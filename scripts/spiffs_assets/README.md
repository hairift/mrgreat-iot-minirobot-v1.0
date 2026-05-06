# Pembuat Aset SPIFFS

Direktori ini berisi utilitas untuk membangun partisi aset SPIFFS pada proyek
ESP32. Berbagai berkas aset dikumpulkan, diproses, lalu dikemas menjadi format
yang siap dipakai firmware di perangkat.

## Fungsi Utama

- memproses model wake word dan model speech recognition
- menyalin font teks bawaan
- memproses kumpulan emoji berbasis PNG atau GIF
- membuat indeks aset otomatis
- menghasilkan berkas akhir `assets.bin`

## Kebutuhan

- Python 3.6 atau lebih baru
- berkas aset yang sesuai dengan konfigurasi proyek

## Cara Pakai

### Sintaks Dasar

```bash
./build.py --wakenet_model <direktori_model_wakenet> \
    --text_font <berkas_font> \
    --emoji_collection <direktori_emoji>
```

### Penjelasan Argumen

| Argumen | Tipe | Wajib | Keterangan |
|---|---|---|---|
| `--wakenet_model` | path direktori | tidak | path ke direktori model wake word |
| `--text_font` | path berkas | tidak | path ke berkas font teks |
| `--emoji_collection` | path direktori | tidak | path ke direktori kumpulan emoji |

### Contoh

```bash
./build.py \
    --wakenet_model ../../managed_components/espressif__esp-sr/model/wakenet_model/wn9_nihaoxiaozhi_tts \
    --text_font ../../components/xiaozhi-fonts/build/font_puhui_common_20_4.bin \
    --emoji_collection ../../components/xiaozhi-fonts/build/emojis_64/
```

```bash
./build.py --text_font ../../components/xiaozhi-fonts/build/font_puhui_common_20_4.bin
```

```bash
./build.py --emoji_collection ../../components/xiaozhi-fonts/build/emojis_64/
```

## Alur Kerja

1. Membuat struktur direktori build:
   `build/`, `build/assets/`, dan `build/output/`.
2. Memproses model speech recognition:
   salin model, buat `srmodels.bin`, lalu pindahkan ke direktori aset.
3. Memproses font teks:
   salin font `.bin` ke direktori aset.
4. Memproses kumpulan emoji:
   salin gambar yang didukung dan buat indeks emoji.
5. Membuat berkas konfigurasi:
   `index.json` dan `config.json`.
6. Mengemas seluruh aset:
   hasil akhir berupa `assets.bin`.

## Berkas Keluaran

Setelah proses selesai, direktori `build/` berisi:

- `assets/` untuk seluruh berkas aset
- `assets.bin` sebagai paket aset akhir
- `config.json` sebagai konfigurasi build
- `output/` sebagai keluaran antara

## Format yang Didukung

- model: `.bin`
- font: `.bin`
- gambar: `.png`, `.gif`
- konfigurasi: `.json`

## Catatan

- Pastikan semua skrip Python pendukung tersedia di direktori ini.
- Gunakan path absolut atau path relatif yang benar terhadap direktori skrip.
- Proses build akan membersihkan hasil build lama.
- Ukuran `assets.bin` tetap harus muat di partisi SPIFFS yang dipakai firmware.
