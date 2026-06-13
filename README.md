# Firmware Mr Great ESP32

Firmware ini adalah versi kustom dari XiaoZhi ESP32 untuk robot mini Mr Great berbasis ESP32-S3. Fokus proyek ini adalah asisten suara bahasa Indonesia, pengetahuan kampus UCIC, pencarian web, audio dua arah, layar OLED, servo kepala dan tangan, serta tombol manual untuk membangunkan asisten.

## Pratinjau Perangkat

![Tampilan robot Mr Great](<docs/v0/mrgreatrobot (1).jpeg>)
![Tampilan robot Mr Great 2](<docs/v0/mrgreatrobot (2).jpeg>)
![Tampilan robot Mr Great 3](<docs/v0/mrgreatrobot (3).jpeg>)
![Tampilan robot Mr Great 4](<docs/v0/mrgreatrobot (4).jpeg>)
![Tampilan robot Mr Great 5](<docs/v0/mrgreatrobot (5).jpeg>)
![Tampilan robot Mr Great 6](<docs/v0/mrgreatrobot (6).jpeg>)
![Tampilan robot Mr Great 7](<docs/v0/mrgreatrobot (7).jpeg>)

## Prioritas Jawaban AI

1. Data kampus lokal di `main/campus_data.cc`.
2. Campus RAG Server jika `CAMPUS_RAG_SERVER_URL` diisi.
3. Pencarian web di `main/web_search.cc` untuk data terbaru.
4. Pengetahuan bawaan model untuk pertanyaan umum.

## Fitur Utama

- Kata bangun lokal `hai hai` untuk memanggil Mr Great.
- Audio masuk melalui INMP441 dan audio keluar melalui MAX98357.
- Layar OLED I2C 0,96 inci untuk status, jaringan, baterai, dan respons.
- Servo kepala, tangan kanan, dan tangan kiri dengan perintah suara serta MCP.
- Tombol tactile 4 kaki di `GPIO1` sebagai tombol manual.
- Bahasa firmware dibatasi ke Indonesia dan Inggris agar aset lebih ringan.
- Ikon non-emosi seperti `microchip_ai` tetap memakai Font Awesome, sedangkan emosi wajah memakai file 64-bit di `components/78__xiaozhi-fonts/src/emoji`.

## Pin Perangkat Saat Ini

| Komponen | Koneksi ESP32-S3 |
|---|---|
| OLED SDA | `GPIO10` |
| OLED SCL | `GPIO9` |
| INMP441 WS | `GPIO4` |
| INMP441 SCK | `GPIO5` |
| INMP441 SD | `GPIO6` |
| MAX98357 DIN | `GPIO7` |
| MAX98357 BCLK | `GPIO15` |
| MAX98357 LRC | `GPIO16` |
| Servo kepala | `GPIO17` |
| Servo tangan kanan | `GPIO38` |
| Servo tangan kiri | `GPIO39` |
| Tombol manual | `GPIO1` ke GND |
| Pembacaan baterai | `GPIO2` melalui pembagi tegangan |
| USB native D- | `GPIO19` |
| USB native D+ | `GPIO20` |

## Catatan USB Type-C

Pin `V` dan `G` pada USB breakout dipakai untuk jalur charging ke TP4056. Pin `D+` dan `D-` bukan jalur daya, jadi tidak akan membuat baterai terisi lebih cepat, tidak menyalakan ESP32, dan tidak terlihat efeknya pada aplikasi normal.

Jika ingin memakai USB data native ESP32-S3, sambungkan `D+` ke `GPIO20`, `D-` ke `GPIO19`, dan pastikan ground USB tersambung ke ground sistem. ESP32-S3 tetap harus mendapat daya dari rangkaian baterai, boost, atau jalur 5V yang aman. Kalau saklar utama mati dan hanya `D+`/`D-` yang tersambung, komputer tidak akan bisa mendeteksi ESP32 karena chip tidak menyala.

Konfigurasi saat ini tetap memakai UART sebagai console utama dan USB Serial/JTAG sebagai console sekunder. Jadi port COM untuk monitor bisa berbeda antara jalur UART bawaan board dan jalur USB native `GPIO19/GPIO20`.

Untuk flash lewat USB native, pin `D+` dan `D-` saja tidak cukup kalau komputer belum melihat port COM baru. Pastikan kabel USB mendukung data, ground USB satu jalur dengan ground ESP32-S3, saklar utama menyala, dan jika perlu masuk mode download dengan menahan tombol BOOT `GPIO0` saat reset atau saat power dinyalakan. Tombol manual `GPIO1` bukan tombol BOOT untuk mode download.

## Catatan Tombol Manual

Tombol manual memakai `GPIO1` ke GND dengan pull-up internal. Untuk tactile 4 kaki, pilih dua kaki yang berbeda sisi atau rail, biasanya satu kaki sisi kiri dan satu kaki sisi kanan atau diagonal. Jangan memakai `kiri atas` + `kiri bawah` atau `kanan atas` + `kanan bawah` jika kedua kaki itu sudah tersambung permanen, karena tekanan tombol tidak akan mengubah sinyal yang dibaca firmware.

## Catatan Baterai

Pembagi tegangan yang direkomendasikan:

- resistor atas `100 kOhm` dari titik baterai ke node ADC
- resistor bawah `100 kOhm` dari node ADC ke GND
- kapasitor `104` atau `100 nF` dari node ADC ke GND
- node ADC masuk ke `GPIO2`

Untuk membaca baterai, titik atas pembagi sebaiknya diambil dari sisi baterai atau output TP4056, bukan dari output boost 5V MT3608. Jika diambil dari output boost, firmware membaca tegangan 5V regulator, bukan tegangan baterai asli.

## Build, Flash, dan Monitor

```powershell
cd E:\MrGreat-IOT-ESP32\MrGreat-esp32-main
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

Jika flash serial kurang stabil, turunkan baud rate:

```powershell
idf.py -p COM3 -b 115200 flash
```

## Dokumentasi Terkait

- [Panduan servo](SERVO_README.md)
- [Konfigurasi BluFi](docs/blufi.md)
- [Campus RAG Server](docs/campus-rag-server.md)
- [Panduan gaya kode](docs/code_style.md)
- [Panduan papan kustom](docs/custom-board.md)
- [Protokol MCP](docs/mcp-protocol.md)
- [Penggunaan MCP](docs/mcp-usage.md)
- [Protokol MQTT dan UDP](docs/mqtt-udp.md)
- [Daftar validasi perangkat keras](docs/production-hardware-checklist.md)
- [Protokol WebSocket](docs/websocket.md)

## Catatan Rilis

File di `managed_components/` adalah dependensi pihak ketiga. File tersebut tidak perlu diubah untuk kustomisasi Mr Great kecuali ada alasan teknis yang jelas.
