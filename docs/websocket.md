# Protokol WebSocket

Dokumen ini merangkum alur komunikasi WebSocket antara perangkat dan peladen.

## Alur Dasar

1. Perangkat melakukan inisialisasi aplikasi dan jaringan.
2. Perangkat membuka koneksi WebSocket ke peladen.
3. Perangkat mengirim pesan `hello`.
4. Peladen membalas `hello` dan memberikan parameter sesi.
5. Audio dan pesan kendali dipertukarkan selama sesi berjalan.
6. Koneksi ditutup saat sesi selesai atau saat terjadi gangguan.

## Pesan Awal

Contoh `hello` dari perangkat:

```json
{
  "type": "hello",
  "version": 1,
  "transport": "websocket",
  "features": {
    "mcp": true
  },
  "audio_params": {
    "format": "opus",
    "sample_rate": 16000,
    "channels": 1,
    "frame_duration": 60
  }
}
```

## Jenis Data

Selama sesi WebSocket, dua jenis data utama dikirim:

- frame audio biner
- pesan JSON teks

Pesan teks dipakai untuk:

- status mendengarkan
- hasil STT
- kendali TTS
- ekspresi emosi
- MCP
- pesan sistem

## Header Umum

Saat membuka koneksi, perangkat biasanya menyertakan:

- `Authorization`
- `Protocol-Version`
- `Device-Id`
- `Client-Id`

## Penutupan Sesi

Sesi berakhir saat:

- perangkat menutup koneksi
- peladen memutus koneksi
- batas waktu atau gangguan jaringan terjadi

Perangkat harus kembali ke status aman setelah kanal audio ditutup.
