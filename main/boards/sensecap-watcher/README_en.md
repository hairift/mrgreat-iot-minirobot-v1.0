# SenseCAP Watcher

Dokumen ini memuat langkah kompilasi varian `SenseCAP Watcher` yang memakai konfigurasi bahasa Inggris pada rilis lama. Isinya kini diseragamkan ke bahasa Indonesia agar konsisten dengan dokumentasi proyek.

## Kompilasi sekali jalan

```bash
python scripts/release.py sensecap-watcher -c config_en.json
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
Xiaozhi Assistant -> Board Type -> SenseCAP Watcher
```

Aktifkan juga opsi tambahan berikut:

```text
CONFIG_BOARD_TYPE_SEEED_STUDIO_SENSECAP_WATCHER=y
CONFIG_ESPTOOLPY_FLASHSIZE_32MB=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions/v2/32m.csv"
CONFIG_BOOTLOADER_CACHE_32BIT_ADDR_QUAD_FLASH=y
CONFIG_ESPTOOLPY_FLASH_MODE_AUTO_DETECT=n
CONFIG_IDF_EXPERIMENTAL_FEATURES=y
CONFIG_LANGUAGE_EN_US=y
CONFIG_SR_WN_WN9_JARVIS_TTS=y
```

## Kompilasi dan flash

```bash
idf.py -DBOARD_NAME=sensecap-watcher-en build flash
```

## Catatan penting

Jika perangkat sebelumnya masih memakai firmware resmi SenseCAP, berhati-hatilah terhadap alamat partisi flash. Penghapusan yang salah dapat merusak data identitas perangkat seperti `EUI`, sehingga perangkat tidak bisa dipulihkan penuh ke layanan `SenseCraft`.

Sebaiknya cadangkan dulu data pabrik dengan perintah berikut:

```bash
esptool.py --chip esp32s3 --baud 2000000 --before default_reset --after hard_reset --no-stub read_flash 0x9000 204800 nvsfactory.bin
```
