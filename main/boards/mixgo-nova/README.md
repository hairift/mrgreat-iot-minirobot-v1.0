# Mixgo Nova

<img src="https://mixly.cn/public/icon/2024/6/09705006c1c643beb96338791ee1dea0_m.png" alt="Mixgo Nova" width="200"/>

`Mixgo Nova` adalah papan pengembangan serbaguna untuk IoT, pendidikan, dan proyek maker. Perangkat ini menggabungkan beberapa sensor dan modul komunikasi nirkabel, serta mendukung pemrograman visual Mixly dan interaksi suara luring.

Referensi perangkat:

- https://mixly.cn/fredqian/mixgo_nova

## Catatan codec ES8374

Jika pengambilan suara dari `CODEC MIC` pada `ES8374` bermasalah, periksa berkas berikut:

```text
managed_components\espressif__esp_codec_dev\device\es8374
```

Potongan fungsi yang perlu diperhatikan:

```c
static int es8374_config_adc_input(audio_codec_es8374_t *codec, es_adc_input_t input)
{
    int ret = 0;
    int reg = 0;
    ret |= es8374_read_reg(codec, 0x21, &reg);
    if (ret == 0) {
        reg = (reg & 0xcf) | 0x24;
        ret |= es8374_write_reg(codec, 0x21, reg);
    }
    return ret;
}
```

Catatan:

- Nilai lama `reg = (reg & 0xcf) | 0x14;`
- Nilai yang dipakai pada dokumentasi ini `reg = (reg & 0xcf) | 0x24;`

## Menentukan target

```bash
idf.py set-target esp32s3
```

## Membuka konfigurasi

```bash
idf.py menuconfig
```

Pilih tipe board `Mixgo Nova` pada menu `Xiaozhi Assistant -> Board Type`.

Pengaturan tambahan yang disarankan:

```text
Component config -> ESP PSRAM -> SPI RAM config -> Mode (QUAD/OCT) -> QUAD Mode PSRAM
Serial flasher config -> Flash size -> 8 MB
Partition Table -> Custom partition CSV file -> partitions/v2/8m.csv
```

## Kompilasi

```bash
idf.py build
```

## Menggabungkan binari

```bash
idf.py merge-bin -o xiaozhi-nova.bin -f raw
```
