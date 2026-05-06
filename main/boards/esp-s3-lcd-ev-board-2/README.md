# ESP-S3 LCD EV Board 2

Pastikan Anda mengetahui versi perangkat keras board yang dipakai. Varian 1.4 dan 1.5 berbeda pada pemetaan IO, sehingga pilihan tipe `ev_board` di konfigurasi harus disesuaikan.

## Perbedaan IO

Perubahan utama antara revisi 1.4 dan 1.5 adalah sebagai berikut:

```text
I2C_SCL   IO18 -> IO48
I2C_SDA   IO8  -> IO47
LCD_DATA6 IO47 -> IO8
LCD_DATA7 IO48 -> IO18
```

## Catatan

- Versi implementasi ini hanya mendukung layar 800 x 480.
- Detail perangkat keras resmi dapat dilihat di:
  `https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-lcd-ev-board/user_guide.html`

## Kompilasi Cepat

```bash
python scripts/release.py esp-s3-lcd-ev-board-2
```
