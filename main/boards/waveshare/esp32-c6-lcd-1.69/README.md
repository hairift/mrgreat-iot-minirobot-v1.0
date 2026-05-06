# Waveshare ESP32-C6 LCD 1.69

Referensi produk:

- https://www.waveshare.net/shop/ESP32-C6-Touch-LCD-1.69.htm
- https://www.waveshare.net/shop/ESP32-C6-LCD-1.69.htm

## Menentukan target

```bash
idf.py set-target esp32c6
```

## Membuka konfigurasi

```bash
idf.py menuconfig
```

Pilih tipe board `esp32-c6-lcd-1.69` pada menu `Xiaozhi Assistant -> Board Type`.

## Kompilasi

```bash
idf.py build
```

## Penulisan Firmware dan Monitor

```bash
idf.py flash monitor
```

## Fungsi tombol

- Tombol `BOOT` sebelum terhubung ke server: masuk mode konfigurasi jaringan
- Tombol `BOOT` setelah terhubung ke server: bangunkan atau interupsi
- Tombol `PWR` klik dua kali: matikan atau nyalakan layar
- Tombol `PWR` tekan lama: hidupkan atau matikan perangkat
