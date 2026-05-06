# EDA-TV-Pro

Papan ini adalah contoh proyek televisi mini dari rangkaian pembelajaran LCEDA.

Referensi proyek:

- https://wiki.lceda.cn/zh-hans/course-projects/smart-internet/tv-pro/tv-pro-introduce.html

## Catatan kompilasi

Contoh proyek ini menggunakan modul ESP32-S3 dengan flash 8 MB. Saat build, gunakan konfigurasi partisi berikut:

```text
Partition Table  --->
  Partition Table (Custom partition table CSV)  --->
  (partitions/v2/8m.csv) Custom partition CSV file
```
