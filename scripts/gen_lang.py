#!/usr/bin/env python3
import argparse
import json
import os

HEADER_TEMPLATE = """// Konfigurasi bahasa yang dibuat otomatis
// Bahasa: {lang_code} dengan cadangan en-US
#pragma once

#include <string_view>

#ifndef {lang_code_for_font}
    #define {lang_code_for_font}  // Bahasa bawaan
#endif

namespace Lang {{
    // Metadata bahasa
    constexpr const char* CODE = "{lang_code}";

    // Sumber daya string, dengan en-US sebagai cadangan jika kunci tidak ada
    namespace Strings {{
{strings}
    }}

    // Sumber daya suara, dengan en-US sebagai cadangan jika file audio tidak ada
    namespace Sounds {{
{sounds}
    }}
}}
"""


def load_base_language(assets_dir):
    """Muat data bahasa dasar en-US."""
    base_lang_path = os.path.join(assets_dir, "locales", "en-US", "language.json")
    if os.path.exists(base_lang_path):
        try:
            with open(base_lang_path, "r", encoding="utf-8") as f:
                base_data = json.load(f)
                print(f"Bahasa dasar en-US dimuat dengan {len(base_data.get('strings', {}))} string")
                return base_data
        except json.JSONDecodeError as e:
            print(f"Peringatan: gagal membaca file bahasa en-US: {e}")
    else:
        print("Peringatan: file bahasa dasar en-US tidak ditemukan, mekanisme cadangan dinonaktifkan")
    return {"strings": {}}


def get_sound_files(directory):
    """Ambil daftar file suara di dalam direktori."""
    if not os.path.exists(directory):
        return []
    return [f for f in os.listdir(directory) if f.endswith(".ogg")]


def generate_header(lang_code, output_path):
    # Turunkan struktur proyek dari jalur keluaran
    # output_path biasanya berupa main/assets/lang_config.h
    main_dir = os.path.dirname(output_path)  # main/assets
    if os.path.basename(main_dir) == "assets":
        main_dir = os.path.dirname(main_dir)  # main
    assets_dir = os.path.join(main_dir, "assets")

    # Susun jalur file JSON bahasa
    input_path = os.path.join(assets_dir, "locales", lang_code, "language.json")

    print(f"Memproses bahasa: {lang_code}")
    print(f"Jalur file input: {input_path}")
    print(f"Jalur file output: {output_path}")

    if not os.path.exists(input_path):
        raise FileNotFoundError(f"File bahasa tidak ditemukan: {input_path}")

    with open(input_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    # Validasi struktur data
    if "language" not in data or "strings" not in data:
        raise ValueError("Struktur JSON tidak valid")

    # Muat data bahasa dasar en-US
    base_data = load_base_language(assets_dir)

    # Gabungkan string: en-US sebagai dasar, bahasa pengguna menimpa isinya
    base_strings = base_data.get("strings", {})
    user_strings = data["strings"]
    merged_strings = base_strings.copy()
    merged_strings.update(user_strings)

    # Statistik
    base_count = len(base_strings)
    user_count = len(user_strings)
    total_count = len(merged_strings)
    fallback_count = total_count - user_count

    print(f"Statistik string bahasa {lang_code}:")
    print(f"  - Bahasa dasar (en-US): {base_count} string")
    print(f"  - Bahasa pengguna: {user_count} string")
    print(f"  - Total: {total_count} string")
    if fallback_count > 0:
        print(f"  - Menggunakan cadangan en-US: {fallback_count} string")

    # Hasilkan konstanta string
    strings = []
    sounds = []
    for key, value in merged_strings.items():
        value = value.replace('"', '\\"')
        strings.append(f'        constexpr const char* {key.upper()} = "{value}";')

    # Kumpulkan file suara: en-US sebagai dasar, bahasa pengguna menimpa isinya
    current_lang_dir = os.path.join(assets_dir, "locales", lang_code)
    base_lang_dir = os.path.join(assets_dir, "locales", "en-US")
    common_dir = os.path.join(assets_dir, "common")

    # Ambil semua file suara yang mungkin dipakai
    base_sounds = get_sound_files(base_lang_dir)
    current_sounds = get_sound_files(current_lang_dir)
    common_sounds = get_sound_files(common_dir)

    # Gabungkan daftar file suara: bahasa pengguna menimpa bahasa dasar
    all_sound_files = set(base_sounds)
    all_sound_files.update(current_sounds)

    # Statistik efek suara
    base_sound_count = len(base_sounds)
    user_sound_count = len(current_sounds)
    common_sound_count = len(common_sounds)
    sound_fallback_count = len(set(base_sounds) - set(current_sounds))

    print(f"Statistik efek suara untuk bahasa {lang_code}:")
    print(f"  - Bahasa dasar (en-US): {base_sound_count} efek suara")
    print(f"  - Bahasa pengguna: {user_sound_count} efek suara")
    print(f"  - Efek suara umum: {common_sound_count} efek suara")
    if sound_fallback_count > 0:
        print(f"  - Menggunakan cadangan en-US: {sound_fallback_count} efek suara")

    # Hasilkan konstanta efek suara khusus bahasa
    for file in sorted(all_sound_files):
        base_name = os.path.splitext(file)[0]
        # Utamakan efek suara bahasa saat ini, jika tidak ada maka gunakan cadangan en-US
        if file in current_sounds:
            sound_lang = lang_code.replace("-", "_").lower()
        else:
            sound_lang = "en_us"

        sounds.append(f'''
        extern const char ogg_{base_name}_start[] asm("_binary_{base_name}_ogg_start");
        extern const char ogg_{base_name}_end[] asm("_binary_{base_name}_ogg_end");
        static const std::string_view OGG_{base_name.upper()} {{
        static_cast<const char*>(ogg_{base_name}_start),
        static_cast<size_t>(ogg_{base_name}_end - ogg_{base_name}_start)
        }};''')

    # Hasilkan konstanta efek suara umum
    for file in sorted(common_sounds):
        base_name = os.path.splitext(file)[0]
        sounds.append(f'''
        extern const char ogg_{base_name}_start[] asm("_binary_{base_name}_ogg_start");
        extern const char ogg_{base_name}_end[] asm("_binary_{base_name}_ogg_end");
        static const std::string_view OGG_{base_name.upper()} {{
        static_cast<const char*>(ogg_{base_name}_start),
        static_cast<size_t>(ogg_{base_name}_end - ogg_{base_name}_start)
        }};''')

    # Isi template
    content = HEADER_TEMPLATE.format(
        lang_code=lang_code,
        lang_code_for_font=lang_code.replace("-", "_").lower(),
        strings="\n".join(sorted(strings)),
        sounds="\n".join(sorted(sounds)),
    )

    # Tulis file keluaran
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(content)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Buat berkas header konfigurasi bahasa dengan cadangan en-US")
    parser.add_argument("--language", required=True, help="Kode bahasa, misalnya zh-CN, en-US, atau ja-JP")
    parser.add_argument("--output", required=True, help="Jalur berkas header keluaran")
    args = parser.parse_args()

    try:
        generate_header(args.language, args.output)
        print(f"Berhasil membuat berkas konfigurasi bahasa: {args.output}")
    except Exception as e:
        print(f"Galat: {e}")
        exit(1)
