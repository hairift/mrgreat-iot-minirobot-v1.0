# Penggunaan MCP untuk Kontrol Perangkat

Dokumen ini menjelaskan cara memakai MCP untuk menemukan dan memanggil alat pada perangkat.

## Ringkasan

MCP memungkinkan peladen:

- membaca daftar alat dari perangkat
- memahami parameter alat
- menjalankan alat tertentu
- menerima hasil dalam format yang konsisten

## Registrasi Alat di Perangkat

Alat biasanya didaftarkan melalui peladen MCP di sisi firmware. Setiap alat sebaiknya memiliki:

- nama yang jelas
- deskripsi singkat
- daftar parameter
- fungsi panggil balik eksekusi

## Contoh Pola Registrasi

```cpp
mcp_server.AddTool(
    "self.audio_speaker.set_volume",
    "Mengatur volume speaker",
    PropertyList({
        Property("volume", kPropertyTypeInteger, 0, 100)
    }),
    [this](const PropertyList& properties) -> ReturnValue {
        return true;
    }
);
```

## Contoh Permintaan

### Mengambil daftar alat

```json
{
  "jsonrpc": "2.0",
  "method": "tools/list",
  "params": { "cursor": "" },
  "id": 1
}
```

### Menjalankan alat

```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "self.audio_speaker.set_volume",
    "arguments": {
      "volume": 80
    }
  },
  "id": 2
}
```

## Rekomendasi

- gunakan nama alat yang stabil
- batasi argumen agar tetap sederhana
- pastikan deskripsi alat jelas untuk model
- kembalikan hasil dalam bentuk yang ringkas dan tegas
- pisahkan alat internal pengguna jika tidak ingin dibaca AI
