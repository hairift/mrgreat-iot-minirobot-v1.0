import os

# Daftar konfigurasi font yang akan dibangkitkan.
font_configs = [
    (14, 1),  # Font ukuran 14, 1 bpp.
    (16, 4),  # Font ukuran 16, 4 bpp.
    (20, 4),  # Font ukuran 20, 4 bpp.
    (30, 1),  # Font ukuran 30, 1 bpp.
    (30, 4),  # Font ukuran 30, 4 bpp.
]


def main():
    # Proses semua konfigurasi font.
    for size, bpp in font_configs:
        print(f"\nSedang membuat font {size}px, {bpp} bpp...")

        # Susun lalu jalankan perintah generator.
        cmd = f"python3 font_awesome.py lvgl --font-size {size} --bpp {bpp}"
        ret = os.system(cmd)

        if ret != 0:
            print(f"Gagal membuat font {size}px {bpp}bpp, kode balik: {ret}")
            return ret

        print(f"Berhasil membuat font_awesome_{size}_{bpp}.c")

    # Buat berkas font_awesome.h.
    cmd = "python3 font_awesome.py generate"
    ret = os.system(cmd)
    if ret != 0:
        print(f"Gagal membuat font_awesome.h, kode balik: {ret}")
        return ret

    return 0


if __name__ == "__main__":
    main()
