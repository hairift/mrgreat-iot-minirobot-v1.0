'''
- Penulis grafis: Copyright 2020 Twitter, Inc dan kontributor lain
- Sumber grafis: https://github.com/twitter/twemoji
- Lisensi grafis: CC-BY 4.0 (https://creativecommons.org/licenses/by/4.0/)
'''

import requests
import os
import sys
import argparse
import cairosvg
from LVGLImage import LVGLImage, ColorFormat, CompressMethod, OutputFormat

# Pemetaan nama emoji ke kode Unicode.
emoji_mapping = {
    "neutral": 0x1f636, # ðŸ˜¶
    "happy": 0x1f642,    # ðŸ™‚
    "laughing": 0x1f606, # ðŸ˜†
    "funny": 0x1f602,    # ðŸ˜‚
    "sad": 0x1f614,      # ðŸ˜”
    "angry": 0x1f620,    # ðŸ˜ 
    "crying": 0x1f62d,   # ðŸ˜­
    "loving": 0x1f60d,   # ðŸ˜
    "embarrassed": 0x1f633, # ðŸ˜³
    "surprised": 0x1f62f,   # ðŸ˜¯
    "shocked": 0x1f631,     # ðŸ˜±
    "thinking": 0x1f914,    # ðŸ¤”
    "winking": 0x1f609,     # ðŸ˜‰
    "cool": 0x1f60e,        # ðŸ˜Ž
    "relaxed": 0x1f60c,     # ðŸ˜Œ
    "delicious": 0x1f924,   # ðŸ¤¤
    "kissy": 0x1f618,       # ðŸ˜˜
    "confident": 0x1f60f,   # ðŸ˜
    "sleepy": 0x1f634,      # ðŸ˜´
    "silly": 0x1f61c,       # ðŸ˜œ
    "confused": 0x1f644     # ðŸ™„
}


# Unduh emoji dari repositori Twemoji lalu simpan ke folder lokal.
def get_emoji_png(name, emoji_utf8, size):
    if not os.path.exists("./build/svg"):
        os.makedirs("./build/svg")

    # Cek apakah berkas SVG sudah tersedia.
    svg_path = f"./build/svg/{name}.svg"
    if not os.path.exists(svg_path):
        url = f"https://raw.githubusercontent.com/twitter/twemoji/refs/heads/master/assets/svg/{emoji_utf8}.svg"
        response = requests.get(url)
        with open(svg_path, "wb") as f:
            f.write(response.content)

    if not os.path.exists(f"./png/twemoji_{size}"):
        os.makedirs(f"./png/twemoji_{size}")

    # Cek apakah berkas PNG dengan ukuran yang diminta sudah ada.
    png_path = f"./png/twemoji_{size}/{name}.png"
    if not os.path.exists(png_path):
        # Gunakan cairosvg untuk mengubah SVG menjadi PNG.
        cairosvg.svg2png(
            url=svg_path,
            write_to=png_path,
            output_width=size,
            output_height=size
        )

    return png_path


def parse_arguments():
    parser = argparse.ArgumentParser(description='Utilitas konverter font emoji')
    parser.add_argument('--size', type=int, default=32, help='Ukuran emoji dalam piksel, bawaan 32')
    parser.add_argument('--type', choices=['png', 'lvgl'], default='png', help='Jenis keluaran: png atau lvgl')
    parser.add_argument('--cf',
                        choices=['I1', 'I2', 'I4', 'I8', 'ARGB8888', 'RGB565A8'],
                        default='RGB565A8',
                        help='Format warna untuk keluaran LVGL, bawaan RGB565A8')
    parser.add_argument('--compress',
                        choices=['NONE', 'RLE', 'LZ4'],
                        default='NONE',
                        help='Metode kompresi untuk keluaran LVGL, bawaan NONE')
    return parser.parse_args()


def generate_lvgl_image(png_path, cf_str, compress_str):
    cf = ColorFormat[cf_str]
    compress = CompressMethod[compress_str]
    img = LVGLImage().from_png(png_path, cf=cf)

    # Buat berkas C.
    c_path = png_path.replace('.png', '.c')
    img.to_c_array(c_path, compress=compress)

    # Buat berkas bin.
    bin_path = png_path.replace('.png', '.bin')
    img.to_bin(bin_path, compress=compress)

    return c_path, bin_path


def main():
    args = parse_arguments()

    # Proses setiap emoji yang didefinisikan.
    for name, code in emoji_mapping.items():
        # Ubah kode heksadesimal menjadi string.
        emoji_utf8 = format(code, 'x')

        # Ambil atau unduh berkas PNG.
        png_path = get_emoji_png(name, emoji_utf8, args.size)

        if args.type == 'lvgl':
            # Buat aset gambar LVGL.
            c_path, bin_path = generate_lvgl_image(png_path, args.cf, args.compress)
            print(f"Generated LVGL files for {name}: {c_path}, {bin_path}")
        else:
            print(f"Generated PNG file for {name}: {png_path}")


if __name__ == "__main__":
    sys.exit(main())
