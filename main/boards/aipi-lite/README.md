# AIPI-Lite

## Kompilasi Cepat

```bash
python scripts/release.py aipi-lite
```

## Kompilasi Manual

Atur target chip:

```bash
idf.py set-target esp32s3
```

Buka konfigurasi:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> AIPI-Lite
```

Kompilasi dan flash:

```bash
idf.py -DBOARD_NAME=aipi-lite build flash
```

## Catatan Penting

Jika perangkat sebelumnya masih memakai firmware asli AiPi-Lite dan bukan varian XiaoZhi, berhati-hatilah saat menangani partisi flash. Jangan sampai informasi perangkat bawaan seperti EUI ikut terhapus, karena hal itu dapat membuat perangkat gagal tersambung kembali ke layanan pabrikan.

Sebelum mem-flash firmware, cadangkan dulu informasi pabrik dengan perintah berikut:

```bash
esptool.py --chip esp32s3 --baud 2000000 --before default_reset --after hard_reset --no-stub read_flash 0x9000 16384 nvsfactory.bin
```
