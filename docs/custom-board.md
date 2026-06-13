# Panduan Papan Kustom Mr Great

Papan aktif untuk robot Mr Great adalah `bread-compact-wifi`. Konfigurasi utama berada di:

```text
main/boards/bread-compact-wifi/config.h
main/boards/bread-compact-wifi/compact_wifi_board.cc
```

## Pin Utama

| Fungsi | GPIO |
|---|---|
| OLED SDA | `GPIO10` |
| OLED SCL | `GPIO9` |
| Mikrofon WS | `GPIO4` |
| Mikrofon SCK | `GPIO5` |
| Mikrofon SD | `GPIO6` |
| Speaker DIN | `GPIO7` |
| Speaker BCLK | `GPIO15` |
| Speaker LRC | `GPIO16` |
| Servo kepala | `GPIO17` |
| Servo tangan kanan | `GPIO38` |
| Servo tangan kiri | `GPIO39` |
| Tombol manual | `GPIO1` |
| ADC baterai | `GPIO2` |
| USB D- | `GPIO19` |
| USB D+ | `GPIO20` |

## USB Native ESP32-S3

`GPIO19` dan `GPIO20` adalah jalur data USB native ESP32-S3. Jalur ini tidak mengisi baterai, tidak menyalakan perangkat, dan tidak bisa menggantikan jalur power. Charging tetap melalui pin `V` dan `G` USB breakout ke TP4056.

Jika ingin komputer mendeteksi ESP32-S3 lewat USB native:

- gunakan kabel USB data, bukan kabel charging saja
- sambungkan ground USB ke ground sistem
- sambungkan `D-` ke `GPIO19`
- sambungkan `D+` ke `GPIO20`
- pastikan `GPIO19` dan `GPIO20` tidak dipakai fungsi lain

Console utama firmware masih UART, sedangkan USB Serial/JTAG aktif sebagai console sekunder. Jika jalur USB native bekerja dan ESP32 sudah mendapat daya, Windows biasanya memunculkan port COM tambahan yang berbeda dari port UART biasa.

Untuk flash dari USB native, gunakan port COM baru tersebut:

```powershell
idf.py -p COMx flash
idf.py -p COMx monitor
```

Jika port COM tidak muncul, cek kabel data, ground bersama, urutan `D+` dan `D-`, driver USB Serial/JTAG, saklar daya utama, serta mode download. Tombol aplikasi `GPIO1` tidak menggantikan tombol BOOT; untuk memaksa download mode tetap gunakan BOOT `GPIO0` ke GND saat reset atau saat power dinyalakan.

## Baterai

Untuk pembacaan baterai, gunakan pembagi tegangan dua resistor `100 kOhm` dan kapasitor `104` ke GND. Node tengah masuk ke `GPIO2`. Titik ukur yang disarankan adalah tegangan baterai atau output TP4056, bukan output boost 5V.

## Tombol Tactile 4 Kaki

Tombol manual memakai `GPIO1` dan GND. Pada tactile 4 kaki, dua kaki pada satu sisi biasanya tersambung permanen, lalu tombol baru menghubungkan sisi kiri dan kanan saat ditekan. Pakai dua kaki yang berbeda sisi atau diagonal. Jika `GPIO1` dan GND dipasang pada `kiri atas` + `kiri bawah` atau `kanan atas` + `kanan bawah` yang satu rail, tekanan tombol tidak akan mengubah sinyal. Firmware akan terasa tidak merespons atau malah membaca tombol selalu aktif.

## Build

```powershell
cd E:\MrGreat-IOT-ESP32\MrGreat-esp32-main
idf.py set-target esp32s3
idf.py build
```
