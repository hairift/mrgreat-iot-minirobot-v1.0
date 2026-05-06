# Protokol MCP

Dokumen ini merangkum alur pertukaran pesan MCP antara perangkat dan peladen agar pemanggilan tool berlangsung rapi dan konsisten.

## Tujuan

MCP dipakai supaya peladen dapat:

- menemukan tool yang tersedia pada perangkat
- memahami parameter yang diterima tiap tool
- memanggil tool secara terstruktur
- menerima hasil atau pesan kesalahan dengan format yang seragam

## Bentuk Pesan

Pesan MCP dibawa oleh protokol transport utama, misalnya WebSocket atau MQTT. Muatan MCP mengikuti pola JSON-RPC.

Contoh sederhana:

```json
{
  "type": "mcp",
  "payload": {
    "jsonrpc": "2.0",
    "method": "tools/list",
    "params": {},
    "id": 1
  }
}
```

## Alur Dasar

1. Perangkat tersambung ke peladen.
2. Perangkat menyiapkan kemampuan MCP yang tersedia.
3. Peladen membuka sesi dengan `initialize`.
4. Peladen meminta daftar tool melalui `tools/list`.
5. Peladen memanggil tool tertentu melalui `tools/call`.
6. Perangkat mengirim hasil eksekusi atau pesan kesalahan.

## Metode Penting

### `initialize`

Dipakai untuk membuka sesi MCP dan saling menyampaikan kemampuan dasar antara kedua sisi.

### `tools/list`

Dipakai untuk meminta daftar tool yang tersedia pada perangkat.

### `tools/call`

Dipakai untuk menjalankan tool tertentu dengan argumen yang sesuai.

## Respons

### Respons berhasil

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "content": [
      { "type": "text", "text": "true" }
    ],
    "isError": false
  }
}
```

### Respons gagal

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "error": {
    "code": -32601,
    "message": "Tool tidak ditemukan"
  }
}
```

## Catatan Implementasi

- nama tool harus stabil, jelas, dan tidak mudah berubah
- skema argumen harus konsisten dengan registrasi tool di firmware
- hasil tool sebaiknya ringkas dan mudah dipahami model
- tool yang hanya boleh dipakai pengguna dapat diberi anotasi khusus agar tidak ditawarkan ke AI
