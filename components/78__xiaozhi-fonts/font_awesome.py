import argparse
import os
import sys


# Pemetaan nama ikon ke kode simbol Font Awesome.
icon_mapping = {
    # Ekspresi wajah.
    "neutral": 0xF5A4,  # Wajah netral
    "happy": 0xF118,  # Wajah senang
    "laughing": 0xF59B,  # Wajah tertawa
    "funny": 0xF588,  # Wajah lucu
    "sad": 0xE384,  # Wajah sedih
    "angry": 0xF556,  # Wajah marah
    "crying": 0xF5B3,  # Wajah menangis
    "loving": 0xF584,  # Wajah penuh cinta
    "embarrassed": 0xF579,  # Wajah malu
    "surprised": 0xE36B,  # Wajah terkejut
    "shocked": 0xE375,  # Wajah syok
    "thinking": 0xE39B,  # Wajah berpikir
    "winking": 0xF4DA,  # Wajah mengedip
    "cool": 0xE398,  # Wajah santai
    "relaxed": 0xE392,  # Wajah rileks
    "delicious": 0xE372,  # Wajah menikmati
    "kissy": 0xF598,  # Wajah mencium
    "confident": 0xE409,  # Wajah percaya diri
    "sleepy": 0xE38D,  # Wajah mengantuk
    "silly": 0xE3A4,  # Wajah usil
    "confused": 0xE36D,  # Wajah bingung

    # Ikon baterai.
    "battery_full": 0xF240,
    "battery_three_quarters": 0xF241,
    "battery_half": 0xF242,
    "battery_quarter": 0xF243,
    "battery_empty": 0xF244,
    "battery_slash": 0xF377,
    "battery_bolt": 0xF376,

    # Ikon Wi-Fi.
    "wifi": 0xF1EB,
    "wifi_fair": 0xF6AB,
    "wifi_weak": 0xF6AA,
    "wifi_slash": 0xF6AC,

    # Ikon kekuatan sinyal.
    "signal": 0xF012,
    "signal_strong": 0xF68F,
    "signal_good": 0xF68E,
    "signal_fair": 0xF68D,
    "signal_weak": 0xF68C,
    "signal_off": 0xF695,

    # Ikon volume.
    "volume_high": 0xF028,
    "volume": 0xF6A8,
    "volume_low": 0xF027,
    "volume_xmark": 0xF6A9,

    # Kendali media.
    "music": 0xF001,
    "check": 0xF00C,
    "xmark": 0xF00D,
    "power_off": 0xF011,
    "gear": 0xF013,
    "trash": 0xF1F8,
    "house": 0xF015,
    "image": 0xF03E,
    "pen_to_square": 0xF044,
    "backward_step": 0xF048,
    "forward_step": 0xF051,
    "play": 0xF04B,
    "pause": 0xF04C,
    "stop": 0xF04D,

    # Arah panah dasar.
    "arrow_left": 0xF060,
    "arrow_right": 0xF061,
    "arrow_up": 0xF062,
    "arrow_down": 0xF063,

    # Ikon umum lainnya.
    "triangle_exclamation": 0xF071,
    "bell": 0xF0F3,
    "location_dot": 0xF3C5,
    "globe": 0xF0AC,
    "location_arrow": 0xF124,
    "sd_card": 0xF7C2,
    "bluetooth": 0xF293,
    "comment": 0xF075,
    "microchip_ai": 0xE1EC,
    "user": 0xF007,
    "user_robot": 0xE04B,
    "download": 0xF019,

    # Tambahan.
    "lock": 0xF023,
    "unlock": 0xF09C,
    "key": 0xF084,
    "link": 0xF0C1,
    "circle_info": 0xF05A,
    "circle_question": 0xF059,
    "circle_check": 0xF058,
    "circle_xmark": 0xF057,
    "clock": 0xF017,
    "alarm_clock": 0xF34E,
    "spinner": 0xF110,
    "temperature_half": 0xF2C9,
    "headphones": 0xF025,
    "microphone": 0xF130,
    "microphone_slash": 0xF131,
    "comment_question": 0xE14B,
    "camera": 0xF030,
    "calendar": 0xF133,
    "envelope": 0xF0E0,
    "brightness": 0xE0C9,
    "phone": 0xF095,
    "compass": 0xF14E,
    "calculator": 0xF1EC,
    "glasses": 0xF530,
    "magnifying_glass": 0xF002,
    "heart": 0xF004,
    "star": 0xF005,
    "gamepad": 0xF11B,
    "watch": 0xF2E1,

    # Arah panah lanjutan.
    "arrows_repeat": 0xF364,
    "arrows_rotate": 0xF021,
    "angle_left": 0xF104,
    "angle_right": 0xF105,
    "angle_up": 0xF106,
    "angle_down": 0xF107,
    "angles_left": 0xF100,
    "angles_right": 0xF101,
    "angles_up": 0xF102,
    "angles_down": 0xF103,
    "cloud_arrow_down": 0xF0ED,  # Unduh dari awan
    "cloud_arrow_up": 0xF0EE,  # Unggah ke awan
    "cloud_slash": 0xE137,

    # Ikon cuaca.
    "sun": 0xF185,  # Cerah
    "moon": 0xF186,  # Bulan
    "cloud": 0xF0C2,  # Awan
    "clouds": 0xF744,  # Mendung
    "cloud_sun": 0xF746,  # Cerah berawan
    "cloud_sun_rain": 0xF743,  # Hujan singkat
    "cloud_moon": 0xF6C3,  # Berawan malam
    "cloud_bolt": 0xF76C,  # Petir
    "cloud_hail": 0xF73A,  # Hujan es
    "cloud_sleet": 0xF741,  # Hujan salju
    "cloud_drizzle": 0xF738,  # Gerimis
    "cloud_moon": 0xF6C3,  # Berawan malam
    "cloud_fog": 0xF74E,  # Kabut
    "cloud_rain": 0xF73D,  # Hujan sedang
    "cloud_showers": 0xF73F,  # Hujan lebat
    "cloud_showers_heavy": 0xF740,  # Hujan sangat lebat
    "snowflake": 0xF2DC,  # Salju
    "snowflakes": 0xF7CF,  # Salju lebat
    "smog": 0xF75F,  # Kabut asap
    "wind": 0xF72E,  # Angin
    "hurricane": 0xF751,  # Badai
    "tornado": 0xF76F,  # Tornado
}


def parse_arguments():
    """Membaca argumen baris perintah."""
    parser = argparse.ArgumentParser(description="Utilitas konversi Font Awesome")
    parser.add_argument(
        "type",
        choices=["lvgl", "dump", "generate"],
        help="Jenis keluaran: lvgl, dump, atau generate",
    )
    parser.add_argument("--font-size", type=int, default=14, help="Ukuran font")
    parser.add_argument("--bpp", type=int, default=4, help="Bit per piksel")
    return parser.parse_args()


def get_font_file(font_size):
    """Memilih berkas font sesuai ukuran yang diminta."""
    if font_size == 30:
        return "./tmp/fa-light-300.otf"
    return "./tmp/fa-regular-400.otf"


def generate_symbols_header_file():
    """Membuat ulang font_awesome.h dan font_awesome.c dari pemetaan ikon."""
    symbols = {}
    for key, value in icon_mapping.items():
        symbols[key] = value

    header_content = """#ifndef FONT_AWESOME_H
#define FONT_AWESOME_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Struktur data simbol
typedef struct {
    const char* name;
    const char* utf8_string;
} font_awesome_symbol_t;

"""

    # Buat definisi makro UTF-8 untuk setiap simbol.
    for key, value in symbols.items():
        char = chr(value)
        utf8 = "".join(f"\\x{byte:02x}" for byte in char.encode("utf-8"))
        header_content += f'#define FONT_AWESOME_{key.upper()} "{utf8}"\n'

    header_content += """
// Deklarasi tabel simbol
extern const font_awesome_symbol_t font_awesome_symbols[];
extern const size_t font_awesome_symbol_count;

// Fungsi bantu inline
static inline const char* font_awesome_get_utf8(const char* name) {
    if (!name) return NULL;

    for (size_t i = 0; i < font_awesome_symbol_count; i++) {
        if (strcmp(font_awesome_symbols[i].name, name) == 0) {
            return font_awesome_symbols[i].utf8_string;
        }
    }
    return NULL;
}

#endif
"""

    header_file = "include/font_awesome.h"
    try:
        with open(header_file, "w", encoding="utf-8") as file_handle:
            file_handle.write(header_content)
        print(f"Berhasil membuat {header_file}")
    except Exception as error:
        print(f"Gagal membuat header: {error}")
        return False

    impl_content = """#include "font_awesome.h"

// Implementasi tabel simbol
const font_awesome_symbol_t font_awesome_symbols[] = {
"""

    # Buat entri tabel simbol satu per satu.
    for key, value in symbols.items():
        char = chr(value)
        utf8 = "".join(f"\\x{byte:02x}" for byte in char.encode("utf-8"))
        impl_content += f'    {{"{key}", "{utf8}"}},\n'

    impl_content += """};

// Jumlah simbol
const size_t font_awesome_symbol_count = sizeof(font_awesome_symbols) / sizeof(font_awesome_symbols[0]);
"""

    impl_file = "src/font_awesome.c"
    try:
        with open(impl_file, "w", encoding="utf-8") as file_handle:
            file_handle.write(impl_content)
        print(f"Berhasil membuat {impl_file}")
        print(f"Total simbol yang dibuat: {len(symbols)}")
        return True
    except Exception as error:
        print(f"Gagal membuat implementasi: {error}")
        return False


def main():
    args = parse_arguments()

    if args.type == "generate":
        return 0 if generate_symbols_header_file() else 1

    symbols = list(icon_mapping.values())
    flags = "--no-compress --no-prefilter --force-fast-kern-format"
    font = get_font_file(args.font_size)

    symbols_str = ",".join(map(hex, symbols))

    if args.type == "lvgl":
        output = f"src/font_awesome_{args.font_size}_{args.bpp}.c"
        cmd = (
            f"lv_font_conv {flags} --font {font} --format lvgl --lv-include lvgl.h "
            f"--bpp {args.bpp} -o {output} --size {args.font_size} -r {symbols_str}"
        )
    else:
        output = "./build/font_awesome_dump"
        import shutil

        shutil.rmtree(output, ignore_errors=True)
        os.makedirs(output, exist_ok=True)
        cmd = (
            f"lv_font_conv {flags} --font {font} --format dump --bpp {args.bpp} "
            f"-o {output} --size {args.font_size} -r {symbols_str}"
        )

    print("Total simbol:", len(symbols))
    print("Membuat keluaran", output)

    ret = os.system(cmd)
    if ret != 0:
        print(f"Perintah gagal dijalankan, kode keluar: {ret}")
        return ret

    print("Perintah selesai dengan sukses")
    return 0


if __name__ == "__main__":
    sys.exit(main())
