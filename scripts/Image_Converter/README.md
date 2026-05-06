# Alat Konversi Gambar LVGL

Direktori ini berisi dua skrip Python untuk mengolah gambar ke format yang
sesuai dengan LVGL.

## `LVGLImage.py`

Skrip ini berasal dari utilitas resmi LVGL dan dipakai sebagai mesin konversi
dasar.

## `lvgl_tools_gui.py`

Skrip ini menyediakan antarmuka grafis untuk memanggil `LVGLImage.py` dan
melakukan konversi gambar secara massal. Alat ini cocok dipakai untuk
mengganti aset visual atau emoji bawaan perangkat.

## Fitur

- antarmuka grafis yang lebih mudah dipakai
- dukungan konversi banyak gambar sekaligus
- pemilihan format warna yang sesuai berdasarkan isi gambar
- dukungan beberapa ukuran keluaran

## Cara Pakai

### Membuat lingkungan virtual

```bash
python -m venv venv
source venv/bin/activate
```

Untuk Windows:

```powershell
venv\Scripts\activate
```

### Memasang dependensi

```bash
pip install -r requirements.txt
```

### Menjalankan alat konversi

```bash
python lvgl_tools_gui.py
```
