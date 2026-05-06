#!/usr/bin/env python3
"""
Membuat font TTF dari karakter yang muncul pada tokenizer model.

Skrip ini memuat satu atau lebih tokenizer, mengambil semua karakter unik dari
gabungan kosakata model, lalu membangun font gabungan yang hanya berisi karakter
tersebut.
"""

import argparse
import os

from fontTools.subset import Options, Subsetter, load_font
from fontTools.ttLib.scaleUpem import scale_upem
from tqdm import tqdm
from transformers import AutoTokenizer


def parse_arguments():
    """Membaca argumen baris perintah."""
    parser = argparse.ArgumentParser(
        description="Membuat font TTF dari karakter tokenizer model"
    )
    parser.add_argument(
        "--max-tokens",
        type=int,
        default=None,
        help="Jumlah token maksimum yang diproses",
    )
    parser.add_argument(
        "--model",
        type=str,
        default="deepseek-ai/DeepSeek-R1,Qwen/Qwen3-235B-A22B-Instruct-2507",
        help="Nama model, pisahkan dengan koma jika lebih dari satu",
    )
    parser.add_argument(
        "--font-style",
        type=str,
        default="Regular",
        help="Gaya font yang dipakai, misalnya Regular atau Medium",
    )
    args = parser.parse_args()

    # Pisahkan daftar model yang ditulis dengan koma.
    model_names = [name.strip() for name in args.model.split(",") if name.strip()]
    return model_names, args.max_tokens, args.font_style


def ensure_build_directory():
    """Membuat direktori build bila belum tersedia."""
    build_dir = "build"
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
        print(f"Direktori dibuat: {build_dir}")
    return build_dir


def process_single_model(model_name, model_idx, total_models, max_tokens, unique_chars):
    """Memproses satu model dan mengekstrak seluruh karakternya."""
    print(f"\n[{model_idx + 1}/{total_models}] Memuat tokenizer dari {model_name}...")
    try:
        tokenizer = AutoTokenizer.from_pretrained(model_name)
        print("Tokenizer berhasil dimuat.")
    except Exception as error:
        print(f"Gagal memuat tokenizer: {error}")
        return False

    vocab_size = len(tokenizer.get_vocab())
    tokens_to_process = max_tokens if max_tokens else vocab_size
    print(f"Memproses {tokens_to_process} token dari total {vocab_size}...")

    for token_id in tqdm(range(min(tokens_to_process, vocab_size)), desc=f"Model {model_idx + 1}"):
        try:
            token = tokenizer.decode(token_id)
            if token.startswith("<｜"):
                continue

            # Simpan ID token terkecil untuk setiap karakter.
            for char in token:
                if char not in unique_chars:
                    unique_chars[char] = token_id
                else:
                    unique_chars[char] = min(unique_chars[char], token_id)
        except Exception as error:
            print(f"Peringatan: gagal mendekode token {token_id}: {error}")
            continue

    return True


def extract_chars_from_models(model_names, max_tokens):
    """Mengambil karakter dari banyak model lalu mengembalikan gabungannya."""
    unique_chars = {}  # Menyimpan karakter dan ID token terkecilnya.

    # Proses semua model dan kumpulkan gabungan karakternya.
    for model_idx, model_name in enumerate(model_names):
        process_single_model(model_name, model_idx, len(model_names), max_tokens, unique_chars)

    print(f"\nTotal karakter unik dari gabungan semua model: {len(unique_chars)}")
    return unique_chars


def save_chars_to_file(unique_chars, build_dir):
    """Menyimpan karakter unik ke berkas sesuai urutan ID token."""
    sorted_chars = sorted(unique_chars.items(), key=lambda item: item[1])

    output_file = os.path.join(build_dir, "chars.txt")
    with open(output_file, "w", encoding="utf-8") as file_handle:
        # Tulis hasil sesuai urutan ID token.
        for char, _token_id in sorted_chars:
            file_handle.write(char + "\n")

    print(f"Menulis {len(unique_chars)} karakter unik ke {output_file}")
    return sorted_chars


def build_font(sorted_chars, font_style):
    """Membangun font dari daftar karakter terurut."""
    print("Menyusun subset font")

    basic_unicodes = set(range(0x20, 0x7F)) | set(range(0xA1, 0x100))
    common_unicodes = basic_unicodes.copy()

    for char, _token_id in sorted_chars:
        common_unicodes.add(ord(char))

    print(f"Jumlah kode Unicode yang dipakai: {len(common_unicodes)}")
    build_ttf("noto", font_style, common_unicodes)


def build_ttf(font_type, font_style, unicodes):
    """Membuat berkas TTF gabungan dari subset beberapa font sumber."""
    # Lokasi dasar direktori font.
    font_base_path = os.path.join(os.path.dirname(__file__), "..")

    font_list = [
        f"{font_base_path}/fonts/Noto_Sans/static/NotoSans-{font_style}.ttf",
        f"{font_base_path}/fonts/Noto_Emoji/static/NotoEmoji-{font_style}.ttf",
        f"{font_base_path}/fonts/Noto_Sans_SC/static/NotoSansSC-{font_style}.ttf",
        f"{font_base_path}/fonts/Noto_Sans_TC/static/NotoSansTC-{font_style}.ttf",
        f"{font_base_path}/fonts/Noto_Sans_JP/static/NotoSansJP-{font_style}.ttf",
        f"{font_base_path}/fonts/Noto_Sans_KR/static/NotoSansKR-{font_style}.ttf",
        f"{font_base_path}/fonts/Noto_Sans_Thai/static/NotoSansThai-{font_style}.ttf",
        f"{font_base_path}/fonts/Noto_Sans_Arabic/static/NotoSansArabic-{font_style}.ttf",
    ]

    if not os.path.exists("build/subsets"):
        os.makedirs("build/subsets")

    if not os.path.exists("ttf"):
        os.makedirs("ttf")

    print(f"Membuat subset dari {len(font_list)} font")

    subset_fonts = []
    subsetter = Subsetter(Options())
    subsetter.populate(unicodes=list(unicodes))

    # Telusuri semua font lalu ambil karakter yang dibutuhkan.
    for font_path in tqdm(font_list):
        if not os.path.exists(font_path):
            print(f"Font {font_path} tidak ditemukan")
            continue

        font = load_font(font_path, Options())
        font_file = font_path.split("/")[-1].split(".")[0]

        subsetter.subset(font)
        scale_upem(font, 1000)

        save_path = f"build/subsets/{font_file}.ttf"
        font.save(save_path)
        subset_fonts.append(save_path)

    # Gabungkan semua font subset ke satu berkas keluaran.
    print(f"Menggabungkan {len(subset_fonts)} font")
    output_path = f"ttf/{font_type}-{font_style}.ttf"

    if subset_fonts:
        cmd = f"fonttools merge {' '.join(subset_fonts)} --output-file={output_path} --drop-tables=vhea,vmtx"
        result = os.system(cmd)

        if result == 0:
            print(f"Font gabungan disimpan ke {output_path}")
        else:
            print(f"Gagal menggabungkan font ke {output_path}")
    else:
        print("Tidak ada font subset yang bisa digabung")


def main():
    model_names, max_tokens, font_style = parse_arguments()
    build_dir = ensure_build_directory()

    unique_chars = extract_chars_from_models(model_names, max_tokens)
    sorted_chars = save_chars_to_file(unique_chars, build_dir)
    build_font(sorted_chars, font_style)


if __name__ == "__main__":
    main()
