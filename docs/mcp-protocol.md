# Protokol MCP

MCP dipakai agar model AI dapat memanggil fungsi perangkat secara terstruktur. Firmware Mr Great memakai MCP untuk data kampus, pencarian web, status perangkat, audio, layar, lampu, dan servo.

## Bentuk Panggilan

Panggilan MCP berisi nama alat dan argumen JSON. Contoh:

```json
{
  "method": "self.campus.query",
  "params": {
    "keyword": "rektor UCIC"
  }
}
```

## Alat Penting

| Alat | Fungsi |
|---|---|
| `self.campus.query` | Mencari jawaban dari data kampus |
| `self.academic.policy` | Mengambil aturan akademik |
| `self.web.search` | Mencari data terbaru dari web |
| `self.servo.power` | Mengaktifkan atau mematikan servo |
| `self.servo.action` | Menjalankan aksi servo |
| `self.audio_speaker.set_volume` | Mengatur volume speaker |
| `self.screen.set_theme` | Mengubah tema tampilan |

## Aturan Jawaban

- Pertanyaan kampus harus mencoba `self.campus.query` terlebih dahulu.
- Pertanyaan data terbaru memakai `self.web.search`.
- Jika alat gagal, model harus menjawab jujur dan tidak mengarang.

## Validasi

Saat monitor berjalan, pastikan log menampilkan pendaftaran alat MCP tanpa duplikasi yang menyebabkan error.
