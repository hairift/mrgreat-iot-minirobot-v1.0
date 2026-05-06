# Zhengchen Cam ML307

## Ringkasan

Papan ini adalah varian Zhengchen Cam dengan dukungan modul 4G ML307. Basis kodenya mengikuti proyek utama XiaoZhi AI, lalu ditambah penyesuaian khusus untuk kamera, audio, dan konektivitas seluler.

## Kompilasi

Disarankan memakai skrip rilis karena papan ini membutuhkan cukup banyak pengaturan `sdkconfig`.

```bash
python ./scripts/release.py zhengchen-cam-ml307
```

Jika ingin kompilasi manual, sesuaikan konfigurasi `menuconfig` dengan isi `zhengchen-cam-ml307/config.json`.

## Penulisan Firmware

```bash
idf.py flash
```

## Alat MCP yang Tersedia

```text
self.get_device_status
self.audio_speaker.set_volume
self.screen.set_brightness
self.screen.set_theme
self.gif.set_gif_mode
self.display.set_mode
self.camera.take_photo
self.AEC.set_mode
self.AEC.get_mode
self.res.esp_restart
```
