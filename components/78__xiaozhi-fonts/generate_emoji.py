import os
import shutil


# Daftar konfigurasi emoji yang akan dibuat.
emoji_configs = [
    (32, "RGB565A8"),   # 32x32
    (64, "RGB565A8"),   # 64x64
    (128, "RGB565A8"),  # 128x128
    (256, "RGB565A8"),  # 256x256
]


def main():
    # Proses semua konfigurasi emoji yang tersedia.
    for size, cf in emoji_configs:
        print(f"\nSedang membuat emoji {size}x{size}, {cf}...")

        # Susun perintah lalu jalankan generator emoji.
        cmd = f"python font_emoji.py --type lvgl --size {size} --cf {cf} --compress NONE"
        ret = os.system(cmd)

        if ret != 0:
            print(f"Gagal membuat emoji {size}x{size}")
            continue

        # Salin build/emoji_*.c ke src/emoji/*.
        src_dir = "./build"
        dst_dir = "./src/emoji"
        if not os.path.exists(dst_dir):
            os.makedirs(dst_dir)
        else:
            # Kosongkan semua berkas di direktori tujuan.
            for file_name in os.listdir(dst_dir):
                file_path = os.path.join(dst_dir, file_name)
                if os.path.isfile(file_path):
                    os.remove(file_path)

        for file_name in os.listdir(src_dir):
            if file_name.startswith("emoji_") and file_name.endswith(".c"):
                shutil.copy(os.path.join(src_dir, file_name), os.path.join(dst_dir, file_name))

        print(f"Berhasil membuat emoji {size}x{size}")


if __name__ == "__main__":
    main()
