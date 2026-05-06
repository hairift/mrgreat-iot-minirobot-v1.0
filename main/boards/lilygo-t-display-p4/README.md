## LILYGO T-Display-P4

`T-Display-P4` adalah papan pengembangan serbaguna berbasis `ESP32-P4`. Papan ini menawarkan layar resolusi tinggi, dukungan periferal yang kaya, dan opsi daya yang fleksibel.

Fitur utamanya:

1. Daya pemrosesan tinggi dengan inti `ESP32-P4`.
2. Desain hemat daya dengan beberapa mode konsumsi.
3. Layar resolusi tinggi, bawaan `540x1168` melalui antarmuka `MIPI`.
4. Periferal bawaan seperti layar sentuh `MIPI`, modul `ESP32-C6`, speaker, mikrofon, `LoRa`, `GPS`, Ethernet, motor getar linear, pengukur baterai mandiri, dan kamera `MIPI`.

Repositori perangkat keras resmi:

- [T-Display-P4](https://github.com/Xinyuan-LilyGO/T-Display-P4)

## Konfigurasi

### Konfigurasi ESP32-P4

- Tentukan target kompilasi:

  ```bash
  idf.py set-target esp32p4
  ```

- Buka `menuconfig`:

  ```bash
  idf.py menuconfig
  ```

- Pilih papan:

  ```text
  Xiaozhi Assistant -> Board Type -> LILYGO T-Display-P4
  ```

- Pilih jenis layar:

  ```text
  Xiaozhi Assistant -> Select the screen type -> (pilih jenis layar yang ingin dipakai)
  ```

- Pilih format warna layar:

  ```text
  Xiaozhi Assistant -> Select the color format of the screen -> (pilih format piksel layar yang dibutuhkan)
  ```

- Kompilasi firmware:

  ```bash
  idf.py build
  ```

### Konfigurasi ESP32-C6

- Flash contoh `slave` dari pustaka `esp-hosted-mcu` untuk target `ESP32-C6`.
- Pastikan versi `esp-hosted-mcu` yang dipakai sama dengan versi yang digunakan oleh `xiaozhi-esp32`.
