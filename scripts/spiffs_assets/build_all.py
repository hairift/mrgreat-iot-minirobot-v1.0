#!/usr/bin/env python3
"""
Bangun beberapa partisi aset SPIFFS dengan kombinasi parameter yang berbeda.

Skrip ini memanggil build.py dengan berbagai kombinasi:
- wakenet_models
- text_fonts
- emoji_collections

Hasil akhirnya berupa berkas assets.bin dengan nama seperti:
wn9_nihaoxiaozhi_tts-font_puhui_common_20_4-emojis_32.bin
"""

import os
import sys
import shutil
import subprocess


def ensure_dir(directory):
    """Pastikan direktori tersedia, buat jika belum ada."""
    os.makedirs(directory, exist_ok=True)


def get_file_path(base_dir, filename):
    """Ambil path penuh berkas dengan menangani kasus `none`."""
    if filename == "none":
        return None
    return os.path.join(base_dir, f"{filename}.bin" if not filename.startswith("emojis_") else filename)


def build_assets(wakenet_model, text_font, emoji_collection, build_dir, final_dir):
    """Bangun assets.bin menggunakan build.py dengan parameter yang diberikan."""

    # Siapkan argumen untuk build.py.
    cmd = [sys.executable, "build.py"]

    if wakenet_model != "none":
        wakenet_path = os.path.join("../../managed_components/espressif__esp-sr/model/wakenet_model", wakenet_model)
        cmd.extend(["--wakenet_model", wakenet_path])

    if text_font != "none":
        text_font_path = os.path.join("../../components/78__xiaozhi-fonts/cbin", f"{text_font}.bin")
        cmd.extend(["--text_font", text_font_path])

    if emoji_collection != "none":
        emoji_path = os.path.join("../../components/xiaozhi-fonts/build", emoji_collection)
        cmd.extend(["--emoji_collection", emoji_path])

    print(f"\nSedang membangun: {wakenet_model}-{text_font}-{emoji_collection}")
    print(f"Menjalankan perintah: {' '.join(cmd)}")

    try:
        # Jalankan build.py.
        subprocess.run(cmd, check=True, cwd=os.path.dirname(__file__))

        # Susun nama berkas keluaran.
        output_name = f"{wakenet_model}-{text_font}-{emoji_collection}.bin"

        # Salin assets.bin yang dihasilkan ke direktori akhir dengan nama baru.
        src_path = os.path.join(build_dir, "assets.bin")
        dst_path = os.path.join(final_dir, output_name)

        if os.path.exists(src_path):
            shutil.copy2(src_path, dst_path)
            print(f"Berhasil membuat: {output_name}")
            return True
        else:
            print("Gagal: berkas assets.bin hasil build tidak ditemukan")
            return False

    except subprocess.CalledProcessError as e:
        print(f"Proses build gagal: {e}")
        return False
    except Exception as e:
        print(f"Terjadi kesalahan tak dikenal: {e}")
        return False


def main():
    # Konfigurasi kombinasi parameter.
    wakenet_models = [
        "none",
        "wn9_nihaoxiaozhi_tts",
        "wn9s_nihaoxiaozhi"
    ]

    text_fonts = [
        "none",
        "font_puhui_common_14_1",
        "font_puhui_common_16_4",
        "font_puhui_common_20_4",
        "font_puhui_common_30_4",
    ]

    emoji_collections = [
        "none",
        "emojis_32",
        "emojis_64",
    ]

    # Ambil direktori tempat skrip berada.
    script_dir = os.path.dirname(os.path.abspath(__file__))

    # Tetapkan path direktori.
    build_dir = os.path.join(script_dir, "build")
    final_dir = os.path.join(build_dir, "final")

    # Pastikan direktori yang dibutuhkan sudah ada.
    ensure_dir(build_dir)
    ensure_dir(final_dir)

    print("Mulai membangun beberapa partisi aset SPIFFS...")
    print(f"Direktori keluaran: {final_dir}")

    # Hitung total kombinasi.
    total_combinations = len(wakenet_models) * len(text_fonts) * len(emoji_collections)

    # Catat jumlah build yang berhasil.
    successful_builds = 0

    # Bangun semua kombinasi yang tersedia.
    for wakenet_model in wakenet_models:
        for text_font in text_fonts:
            for emoji_collection in emoji_collections:
                if build_assets(wakenet_model, text_font, emoji_collection, build_dir, final_dir):
                    successful_builds += 1

    print("\nProses build selesai.")
    print(f"Build berhasil: {successful_builds}/{total_combinations}")
    print(f"Lokasi berkas keluaran: {final_dir}")

    # Tampilkan daftar berkas yang berhasil dibuat.
    if os.path.exists(final_dir):
        files = [f for f in os.listdir(final_dir) if f.endswith('.bin')]
        if files:
            print("\nBerkas yang dihasilkan:")
            for file in sorted(files):
                file_size = os.path.getsize(os.path.join(final_dir, file))
                print(f"  {file} ({file_size:,} bytes)")
        else:
            print("\nTidak ada berkas .bin yang ditemukan")


if __name__ == "__main__":
    main()
