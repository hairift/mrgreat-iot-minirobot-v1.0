# Zhengchen 1.54 TFT ML307

## Ringkasan

Papan ini adalah varian Zhengchen 1.54 TFT dengan modul jaringan ML307. Implementasi utamanya berada di file sesuai folder board ini dan terhubung dengan manajemen daya, layar, dan pengendali MCP.

## Fitur Utama

- Layar TFT 1,54 inci
- Dukungan jaringan melalui modul ML307
- Tombol fisik untuk percakapan dan volume
- Integrasi alat MCP dan manajemen daya

## Langkah Konfigurasi

1. Atur target build ke target yang sesuai untuk perangkat ini.
2. Buka `menuconfig`.
3. Pilih tipe papan sesuai nama `zhengchen-1.54tft-ml307`.
4. Sesuaikan pengaturan jaringan, audio, layar, dan daya sesuai perangkat keras.

## Kompilasi dan Flash

```bash
idf.py menuconfig
idf.py build
idf.py flash
idf.py monitor
```

## Catatan

- Perilaku alat MCP dan jaringan tambahan mengikuti implementasi terbaru pada source.
- Gunakan source board ini sebagai acuan utama jika ada perbedaan antar revisi perangkat keras.
