# SenseCAP Watcher

Dokumentasi ini memakai alur build terbaru untuk `SenseCAP Watcher`.

## Kompilasi sekali jalan

```bash
python scripts/release.py sensecap-watcher
```

## Konfigurasi manual

Tentukan target:

```bash
idf.py set-target esp32s3
```

Buka konfigurasi:

```bash
idf.py menuconfig
```

Pilih papan berikut:

```text
Xiaozhi Assistant -> Board Type -> SenseCAP Watcher
```

Pastikan opsi tambahan berikut juga aktif di `menuconfig`:

```text
CONFIG_BOARD_TYPE_SEEED_STUDIO_SENSECAP_WATCHER=y
CONFIG_ESPTOOLPY_FLASHSIZE_32MB=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions/v2/32m.csv"
CONFIG_BOOTLOADER_CACHE_32BIT_ADDR_QUAD_FLASH=y
CONFIG_ESPTOOLPY_FLASH_MODE_AUTO_DETECT=n
CONFIG_IDF_EXPERIMENTAL_FEATURES=y
```

## Kompilasi dan flash

```bash
idf.py -DBOARD_NAME=sensecap-watcher build flash
```

## Catatan penting

Jika perangkat sebelumnya dikirim dengan firmware resmi SenseCAP, berhati-hatilah terhadap alamat partisi flash. Kesalahan penghapusan dapat merusak informasi perangkat seperti EUI, sehingga perangkat tidak bisa kembali terhubung normal ke server SenseCraft. Sebelum flash, simpan dulu informasi penting perangkat agar ada jalur pemulihan.

Contoh perintah untuk mencadangkan informasi pabrik:

```bash
esptool.py --chip esp32s3 --baud 2000000 --before default_reset --after hard_reset --no-stub read_flash 0x9000 204800 nvsfactory.bin
```
