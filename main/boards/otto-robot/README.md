# Otto Robot

<p align="center">
  <img width="80%" align="center" src="../../../docs/V1/otto-robot.png" alt="Otto Robot">
</p>

## Ringkasan

Otto Robot adalah robot bipedal berbasis ESP32 yang mendukung percakapan suara dan berbagai gerakan interaktif. Proyek ini menambahkan integrasi asisten Xiaozhi ke platform Otto agar robot dapat bergerak, merespons perintah, dan menampilkan karakter yang lebih hidup.

Referensi tambahan:

- Situs proyek: `www.ottodiy.tech`
- Perangkat keras: `https://oshwhub.com/txp666/ottorobot`

## Kontrol tambahan

Jika ingin memakai mini program WeChat yang sudah disediakan di dokumentasi lama, berkas gambar QR masih tersedia pada file:

- `main/boards/otto-robot/README.md` versi sebelumnya
- `main/boards/otto-robot/otto` pada aset dokumentasi terkait bila dibutuhkan tim

## Saran konfigurasi role

Contoh karakter yang cocok untuk Otto:

- Otto adalah robot kecil berkaki dua yang ramah dan ekspresif.
- Otto mampu berjalan, berputar, melompat, bergoyang, serta melakukan berbagai gerakan tangan jika servo tangan dipasang.
- Otto boleh memakai gerakan untuk memperkuat respons, tetapi tetap harus menjaga gerakan aman dan sesuai konteks.
- Saat memberi salam, Otto bisa melambaikan tangan.
- Saat senang, Otto bisa bergoyang atau mengangkat tangan.
- Saat berpamitan, Otto bisa melambaikan tangan.

## Ringkasan kemampuan

Gerakan utama yang didukung:

- `walk`
- `turn`
- `jump`
- `swing`
- `moonwalk`
- `bend`
- `shake_leg`
- `updown`
- `whirlwind_leg`
- `sit`
- `showcase`
- `home`

Gerakan tangan yang tersedia bila servo tangan dipasang:

- `hands_up`
- `hands_down`
- `hand_wave`
- `windmill`
- `takeoff`
- `fitness`
- `greeting`
- `shy`
- `radio_calisthenics`
- `magic_circle`

## Tool utama

Tool gerakan utama:

- `self.otto.action`

Parameter penting:

- `action`: nama gerakan
- `steps`: jumlah langkah atau pengulangan
- `speed`: kecepatan gerakan, makin kecil nilainya makin cepat
- `direction`: arah gerakan
- `amount`: besar amplitudo gerakan
- `arm_swing`: amplitudo ayunan tangan untuk gerakan tertentu

Tool sistem yang tersedia:

- `self.otto.stop`
- `self.otto.get_status`
- `self.otto.set_trim`
- `self.otto.get_trims`
- `self.otto.get_ip`
- `self.battery.get_level`
- `self.otto.servo_sequences`

## Panduan parameter

Saran umum:

- `speed 1200-1500` untuk gerakan pelan dan presisi
- `speed 900-1200` untuk penggunaan harian
- `speed 500-800` untuk gerakan cepat
- `amount 10-30` untuk gerakan kecil
- `amount 30-60` untuk gerakan sedang
- `amount 60-120` untuk gerakan besar

Catatan perilaku:

- Sebagian besar gerakan akan kembali ke posisi awal setelah selesai.
- `sit` dan `showcase` tidak selalu langsung kembali ke posisi awal.
- Gerakan dijalankan di tugas latar belakang sehingga perintah baru masih bisa diterima.
- Jika servo tangan tidak dipasang, gerakan tangan akan dilewati.

## Contoh pemanggilan tool

```json
{"name": "self.otto.action", "arguments": {"action": "walk"}}
{"name": "self.otto.action", "arguments": {"action": "turn", "steps": 2, "arm_swing": 100}}
{"name": "self.otto.action", "arguments": {"action": "jump", "steps": 1, "speed": 1000}}
{"name": "self.otto.action", "arguments": {"action": "showcase"}}
{"name": "self.otto.action", "arguments": {"action": "home"}}
{"name": "self.otto.stop", "arguments": {}}
{"name": "self.otto.get_ip", "arguments": {}}
```

## Contoh perintah suara

- berjalan maju
- belok kiri
- lompat
- moonwalk
- duduk
- lambaikan tangan
- lakukan showcase
- berhenti

## Catatan

Perintah `home` tetap dipanggil melalui `self.otto.action` dengan nilai `{"action":"home"}`.
