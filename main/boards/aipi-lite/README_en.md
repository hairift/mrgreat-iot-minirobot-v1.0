# AiPi-Lite

Dokumen ini memuat langkah kompilasi varian `AiPi-Lite` yang sebelumnya dipisahkan sebagai dokumentasi bahasa Inggris. Isinya kini diseragamkan ke bahasa Indonesia.

## Kompilasi sekali jalan

```bash
python scripts/release.py aipi-lite -c config_en.json
```

## Konfigurasi manual

Tentukan target:

```bash
idf.py set-target esp32s3
```

Buka `menuconfig`:

```bash
idf.py menuconfig
```

Pilih papan:

```text
Xiaozhi Assistant -> Board Type -> AiPi-Lite
```

## Kompilasi dan flash

```bash
idf.py -DBOARD_NAME=aipi-lite build flash
```

## Catatan penting

Jika perangkat sebelumnya masih memakai firmware bawaan `AiPi-Lite`, berhati-hatilah terhadap alamat partisi flash. Penghapusan yang salah dapat merusak data identitas perangkat dan menyulitkan proses pemulihan.

Cadangan data pabrik dapat dibuat dengan perintah berikut:

```bash
esptool.py --chip esp32s3 --baud 2000000 --before default_reset --after hard_reset --no-stub read_flash 0x9000 16384 nvsfactory.bin
```
