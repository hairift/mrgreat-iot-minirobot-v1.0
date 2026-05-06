# Panduan Servo Mr Great

## Gambaran Umum

Dokumen ini menjelaskan koneksi perangkat keras, batas gerak, perintah suara, dan alat MCP untuk sistem servo pada perangkat Mr Great.

## Konfigurasi Perangkat Keras

### Layar ST7735 1.44 inci

| Pin Layar | ESP32-S3 |
|---|---|
| BLK | G3 |
| CS | GND |
| DC | G46 |
| RES | G11 |
| SDA | G10 |
| SCL | G9 |
| VCC | 3V3 |

### Speaker MAX98357

| Pin Speaker | ESP32-S3 |
|---|---|
| LRC | G16 |
| BCLK | G15 |
| DIN | G7 |
| GAIN | GND |
| SD | 3V3 |
| GND | GND |
| Vin | 3V3 |

### Mikrofon INMP441

| Pin Mikrofon | ESP32-S3 |
|---|---|
| GND | GND |
| L/R | GND |
| WS | G4 |
| VDD | 3V3 |
| SD | G6 |
| SCK | G5 |

### Servo SG90

| Aktuator | GPIO | Gerakan | Inversi |
|---|---|---|---|
| Kepala | 23 | kiri-kanan | tidak |
| Tangan kanan | 13 | atas-bawah | tidak |
| Tangan kiri | 22 | atas-bawah | ya |

Kepala hanya mendukung gerakan kiri-kanan. Kepala tidak mendukung angguk naik-turun.

## Posisi Tangan

| Posisi | Keterangan |
|---|---|
| Atas | posisi tertinggi |
| Lurus | posisi horizontal |
| Bawah | posisi istirahat |

## Perintah Suara

### Daya servo

- `hidupkan servo`
- `servo on`
- `aktifkan servo`
- `matikan servo`
- `servo off`
- `hentikan servo`

### Reset posisi

- `reset`
- `posisi semula`
- `posisi awal`
- `netral`
- `diam`

### Kepala

- `lihat kanan`
- `lihat kiri`
- `lihat depan`
- `gelengkan kepala`

### Tangan

- `angkat tangan kanan`
- `angkat tangan kiri`
- `tangan kanan lurus`
- `tangan kiri lurus`
- `turunkan tangan kanan`
- `turunkan tangan kiri`
- `angkat kedua tangan`

### Aksi interaktif

- `lambaikan tangan kanan`
- `lambaikan tangan kiri`
- `lambaikan tangan`
- `hormat`
- `salam`
- `tepuk tangan`
- `halo`
- `hai`
- `menyapa`
- `joget`
- `menari`

## Emosi yang Didukung

Sistem servo mendukung emosi otomatis seperti:

- netral
- senang
- tertawa
- sedih
- marah
- menangis
- sayang
- malu
- terkejut
- berpikir
- percaya diri
- mengantuk
- bingung

Gerakan emosi harus tetap tunduk pada status servo aktif atau nonaktif.

## Alat MCP Servo

```text
self.servo.enable
self.servo.disable
self.servo.shake_head
self.servo.turn_head_right
self.servo.turn_head_left
self.servo.head_center
self.servo.wave_right
self.servo.wave_left
self.servo.wave_both
self.servo.raise_right
self.servo.raise_left
self.servo.raise_both
self.servo.straight_right
self.servo.straight_left
self.servo.lower_right
self.servo.lower_left
self.servo.dance
self.servo.salam
self.servo.hormat
self.servo.tepuk_tangan
self.servo.menyapa
self.servo.reset
```

## Kompilasi dan Unggah

```powershell
cd E:\MrGreat-IOT-ESP32\MrGreat-esp32-main
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

## File Terkait

- `main/servo_controller.h`
- `main/servo_controller.cc`
- `main/servo_mcp_tools.h`
- `main/boards/bread-compact-wifi-lcd/config.h`
- `main/boards/bread-compact-wifi-lcd/compact_wifi_board_lcd.cc`
