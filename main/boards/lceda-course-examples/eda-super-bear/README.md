# EDA-Super-Bear

Papan ini adalah contoh proyek robot beruang dari rangkaian pembelajaran LCEDA.

Referensi proyek:

- https://wiki.lceda.cn/zh-hans/course-projects/smart-internet/eda-superbear/eda-superbear-introduce.html

## Catatan kompilasi

Ukuran flash mengikuti kapasitas modul ESP32-S3 yang dipakai. Jika modul memakai flash 8 MB, gunakan pengaturan berikut:

```text
Partition Table  --->
  Partition Table (Custom partition table CSV)  --->
  (partitions/v2/8m.csv) Custom partition CSV file
```
