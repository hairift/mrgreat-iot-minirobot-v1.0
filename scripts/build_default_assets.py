#!/usr/bin/env python3
"""
Bangun aset bawaan berdasarkan konfigurasi.

Skrip ini membaca konfigurasi dari sdkconfig lalu membuat assets.bin yang sesuai
dengan konfigurasi board saat ini.

Cara pakai:
    ./build_default_assets.py --sdkconfig <path> --builtin_text_font <nama_font> \
        --default_emoji_collection <nama_koleksi> --output <path_keluaran>
"""

import argparse
import io
import os
import shutil
import sys
import json
import struct
import time
from datetime import datetime


# =============================================================================
# Fungsi pengemasan model (diambil dari pack_model.py)
# =============================================================================

def remove_tree_with_retry(path, required=False, attempts=8, delay_seconds=0.15):
    """
    Hapus direktori dengan retry singkat.

    Di Windows, proses antivirus, indexer, atau tool build kadang masih memegang
    berkas sementara selama beberapa milidetik. Cleanup akhir tidak boleh membuat
    build gagal jika assets.bin sebenarnya sudah berhasil dibuat.
    """
    if not os.path.exists(path):
        return True

    last_error = None
    for attempt in range(attempts):
        try:
            shutil.rmtree(path)
            return True
        except OSError as exc:
            last_error = exc
            time.sleep(delay_seconds * (attempt + 1))

    if required:
        raise last_error

    print(f"Warning: gagal membersihkan direktori sementara setelah retry: {path} ({last_error})")
    return False

def struct_pack_string(string, max_len=None):
    """
    Kemas string menjadi data biner.

    Jika max_len bernilai None, panjang keluaran mengikuti panjang string.
    Jika tidak, sisa ruang akan diisi padding melalui struct.pack('x').
    """
    if max_len == None :
        max_len = len(string)
    else:
        assert len(string) <= max_len

    left_num = max_len - len(string)
    out_bytes = None
    for char in string:
        if out_bytes == None:
            out_bytes = struct.pack('b', ord(char))
        else:
            out_bytes += struct.pack('b', ord(char))
    for i in range(left_num):
        out_bytes += struct.pack('x')
    return out_bytes


def read_data(filename):
    """Baca data biner seperti berkas indeks dan mndata."""
    data = None
    with open(filename, "rb") as f:
        data = f.read()
    return data


def pack_models(model_path, out_file="srmodels.bin"):
    """
    Kemas semua model menjadi satu berkas biner dengan format berikut:
    {
        model_num: int
        model1_info: model_info_t
        model2_info: model_info_t
        ...
        model1_index,model1_data,model1_MODEL_INFO
        model1_index,model1_data,model1_MODEL_INFO
        ...
    }model_pack_t

    {
        model_name: char[32]
        file_number: int
        file1_name: char[32]
        file1_start: int  
        file1_len: int 
        file2_name: char[32]
        file2_start: int   // panjang_data = posisi_info - posisi_data
        file2_len: int
        ...
    }model_info_t
    """
    models = {}
    file_num = 0
    model_num = 0
    for root, dirs, _ in os.walk(model_path):
        for model_name in dirs:
            models[model_name] = {}
            model_dir = os.path.join(root, model_name)
            model_num += 1
            for _, _, files in os.walk(model_dir):
                for file_name in files:
                    file_num += 1
                    file_path = os.path.join(model_dir, file_name)
                    models[model_name][file_name] = read_data(file_path)
    
    model_num = len(models)
    header_len = 4 + model_num*(32+4) + file_num*(32+4+4) 
    out_bin = struct.pack('I', model_num)  # jumlah model
    data_bin = None
    for key in models:
        model_bin = struct_pack_string(key, 32) # + nama model
        model_bin += struct.pack('I', len(models[key])) # + jumlah file pada model ini
        
        for file_name in models[key]:
            model_bin += struct_pack_string(file_name, 32) # + nama file
            if data_bin == None:
                model_bin += struct.pack('I', header_len) 
                data_bin = models[key][file_name]
                model_bin += struct.pack('I', len(models[key][file_name]))
            else:
                model_bin += struct.pack('I', header_len+len(data_bin))
                data_bin += models[key][file_name]
                model_bin += struct.pack('I', len(models[key][file_name]))
        
        out_bin += model_bin
    assert len(out_bin) == header_len
    if data_bin != None:
        out_bin += data_bin

    out_file = os.path.join(model_path, out_file)
    with open(out_file, "wb") as f:
        f.write(out_bin)


# =============================================================================
# Fungsi pembuatan aset (diambil dari build.py)
# =============================================================================

def ensure_dir(directory):
    """Pastikan direktori tersedia, buat jika belum ada."""
    os.makedirs(directory, exist_ok=True)


def copy_file(src, dst):
    """Salin berkas."""
    if os.path.exists(src):
        shutil.copy2(src, dst)
        print(f"Disalin: {src} -> {dst}")
        return True
    else:
        print(f"Peringatan: Berkas sumber tidak ada: {src}")
        return False


def copy_directory(src, dst):
    """Salin direktori."""
    if os.path.exists(src):
        shutil.copytree(src, dst, dirs_exist_ok=True)
        print(f"Direktori disalin: {src} -> {dst}")
        return True
    else:
        print(f"Peringatan: Direktori sumber tidak ada: {src}")
        return False


def process_sr_models(wakenet_model_dirs, multinet_model_dirs, build_dir, assets_dir):
    """Proses model SR, baik wakenet maupun multinet, lalu buat srmodels.bin."""
    if not wakenet_model_dirs and not multinet_model_dirs:
        return None
    
    # Buat direktori build untuk model SR.
    sr_models_build_dir = os.path.join(build_dir, "srmodels")
    if os.path.exists(sr_models_build_dir):
        shutil.rmtree(sr_models_build_dir)
    os.makedirs(sr_models_build_dir)
    
    models_processed = 0
    
    # Salin model wakenet jika tersedia.
    if wakenet_model_dirs:
        for wakenet_model_dir in wakenet_model_dirs:
            wakenet_name = os.path.basename(wakenet_model_dir)
            wakenet_dst = os.path.join(sr_models_build_dir, wakenet_name)
            if copy_directory(wakenet_model_dir, wakenet_dst):
                models_processed += 1
                print(f"Model wakenet ditambahkan: {wakenet_name}")
    
    # Salin model multinet jika tersedia.
    if multinet_model_dirs:
        for multinet_model_dir in multinet_model_dirs:
            multinet_name = os.path.basename(multinet_model_dir)
            multinet_dst = os.path.join(sr_models_build_dir, multinet_name)
            if copy_directory(multinet_model_dir, multinet_dst):
                models_processed += 1
                print(f"Model multinet ditambahkan: {multinet_name}")
    
    if models_processed == 0:
        print("Peringatan: Tidak ada model SR yang berhasil diproses")
        return None
    
    # Gunakan fungsi pack_models untuk membuat srmodels.bin.
    srmodels_output = os.path.join(sr_models_build_dir, "srmodels.bin")
    try:
        pack_models(sr_models_build_dir, "srmodels.bin")
        print(f"Dihasilkan: {srmodels_output}")
        # Salin srmodels.bin ke direktori aset.
        copy_file(srmodels_output, os.path.join(assets_dir, "srmodels.bin"))
        return "srmodels.bin"
    except Exception as e:
        print(f"Error: Gagal membuat srmodels.bin: {e}")
        return None


def process_text_font(text_font_file, assets_dir):
    """Proses parameter text_font."""
    if not text_font_file:
        return None
    
    # Salin berkas masukan ke build/assets.
    font_filename = os.path.basename(text_font_file)
    font_dst = os.path.join(assets_dir, font_filename)
    if copy_file(text_font_file, font_dst):
        return font_filename
    return None


def process_emoji_collection(emoji_collection_dir, assets_dir):
    """Proses parameter emoji_collection."""
    if not emoji_collection_dir:
        return []
    
    emoji_list = []
    
    # Cek apakah koleksi yang dipakai adalah otto-gif.
    is_otto_gif = 'otto-emoji-gif-component' in emoji_collection_dir or emoji_collection_dir.endswith('otto-gif')
    
    # Pemetaan alias emoji untuk koleksi otto GIF.
    otto_gif_aliases = {
        "staticstate": ["neutral", "relaxed", "sleepy", "idle"],
        "happy": ["laughing", "funny", "loving", "confident", "winking", "cool", "delicious", "kissy", "silly"],
        "sad": ["crying"],
        "anger": ["angry"],
        "scare": ["surprised", "shocked"],
        "buxue": ["thinking", "confused", "embarrassed"]
    }
    
    # Salin setiap gambar dari direktori masukan ke build/assets.
    for root, dirs, files in os.walk(emoji_collection_dir):
        for file in files:
            if file.lower().endswith(('.png', '.gif')):
                # Salin berkas
                src_file = os.path.join(root, file)
                dst_file = os.path.join(assets_dir, file)
                if copy_file(src_file, dst_file):
                    # Ambil nama berkas tanpa ekstensi.
                    filename_without_ext = os.path.splitext(file)[0]
                    
                    # Tambahkan entri emoji utama.
                    emoji_list.append({
                        "name": filename_without_ext,
                        "file": file
                    })
                    
                    # Tambahkan alias untuk emoji otto-gif.
                    if is_otto_gif and filename_without_ext in otto_gif_aliases:
                        for alias in otto_gif_aliases[filename_without_ext]:
                            emoji_list.append({
                                "name": alias,
                                "file": file
                            })
    
    return emoji_list


def process_extra_files(extra_files_dir, assets_dir):
    """Proses parameter default_assets_extra_files."""
    if not extra_files_dir:
        return []
    
    if not os.path.exists(extra_files_dir):
        print(f"Warning: Extra files directory not found: {extra_files_dir}")
        return []
    
    extra_files_list = []
    
    # Salin setiap berkas dari direktori masukan ke build/assets.
    for root, dirs, files in os.walk(extra_files_dir):
        for file in files:
            # Lewati berkas dan direktori tersembunyi.
            if file.startswith('.'):
                continue
                
            # Salin berkas.
            src_file = os.path.join(root, file)
            dst_file = os.path.join(assets_dir, file)
            if copy_file(src_file, dst_file):
                extra_files_list.append(file)
    
    if extra_files_list:
        print(f"Processed {len(extra_files_list)} extra files from: {extra_files_dir}")
    
    return extra_files_list


def generate_index_json(assets_dir, srmodels, text_font, emoji_collection, extra_files=None, multinet_model_info=None):
    """Buat berkas index.json."""
    index_data = {
        "version": 1
    }
    
    if srmodels:
        index_data["srmodels"] = srmodels
    
    if text_font:
        index_data["text_font"] = text_font
    
    if emoji_collection:
        index_data["emoji_collection"] = emoji_collection
    
    if extra_files:
        index_data["extra_files"] = extra_files
    
    if multinet_model_info:
        index_data["multinet_model"] = multinet_model_info
    
    # Tulis index.json.
    index_path = os.path.join(assets_dir, "index.json")
    with open(index_path, 'w', encoding='utf-8') as f:
        json.dump(index_data, f, indent=4, ensure_ascii=False)
    
    print(f"Generated: {index_path}")


def generate_config_json(build_dir, assets_dir):
    """Buat berkas config.json."""
    config_data = {
        "include_path": os.path.join(build_dir, "include"),
        "assets_path": assets_dir,
        "image_file": os.path.join(build_dir, "output", "assets.bin"),
        "lvgl_ver": "9.3.0",
        "assets_size": "0x400000",
        "support_format": ".png, .gif, .jpg, .bin, .json",
        "name_length": "32",
        "split_height": "0",
        "support_qoi": False,
        "support_spng": False,
        "support_sjpg": False,
        "support_sqoi": False,
        "support_raw": False,
        "support_raw_dither": False,
        "support_raw_bgr": False
    }
    
    # Tulis config.json.
    config_path = os.path.join(build_dir, "config.json")
    with open(config_path, 'w', encoding='utf-8') as f:
        json.dump(config_data, f, indent=4, ensure_ascii=False)
    
    print(f"Generated: {config_path}")
    return config_path


# =============================================================================
# Pembuatan aset SPIFFS versi ringkas (diambil dari spiffs_assets_gen.py)
# =============================================================================

def compute_checksum(data):
    checksum = sum(data) & 0xFFFF
    return checksum


def sort_key(filename):
    basename, extension = os.path.splitext(filename)
    return extension, basename


def pack_assets_simple(target_path, include_path, out_file, assets_path, max_name_len=32):
    """
    Versi ringkas dari pack_assets untuk pengemasan berkas dasar.
    """
    merged_data = bytearray()
    file_info_list = []
    skip_files = ['config.json']

    # Pastikan direktori keluaran sudah ada.
    os.makedirs(os.path.dirname(out_file), exist_ok=True)
    os.makedirs(include_path, exist_ok=True)

    file_list = sorted(os.listdir(target_path), key=sort_key)
    for filename in file_list:
        if filename in skip_files:
            continue

        file_path = os.path.join(target_path, filename)
        if not os.path.isfile(file_path):
            continue
            
        file_name = os.path.basename(file_path)
        file_size = os.path.getsize(file_path)

        file_info_list.append((file_name, len(merged_data), file_size, 0, 0))
        # Tambahkan prefiks 0x5A5A ke merged_data.
        merged_data.extend(b'\x5A' * 2)

        with open(file_path, 'rb') as bin_file:
            bin_data = bin_file.read()

        merged_data.extend(bin_data)

    total_files = len(file_info_list)

    mmap_table = bytearray()
    for file_name, offset, file_size, width, height in file_info_list:
        if len(file_name) > max_name_len:
            print(f'Peringatan: "{file_name}" melebihi {max_name_len} byte dan akan dipotong.')
        fixed_name = file_name.ljust(max_name_len, '\0')[:max_name_len]
        mmap_table.extend(fixed_name.encode('utf-8'))
        mmap_table.extend(file_size.to_bytes(4, byteorder='little'))
        mmap_table.extend(offset.to_bytes(4, byteorder='little'))
        mmap_table.extend(width.to_bytes(2, byteorder='little'))
        mmap_table.extend(height.to_bytes(2, byteorder='little'))

    combined_data = mmap_table + merged_data
    combined_checksum = compute_checksum(combined_data)
    combined_data_length = len(combined_data).to_bytes(4, byteorder='little')
    header_data = total_files.to_bytes(4, byteorder='little') + combined_checksum.to_bytes(4, byteorder='little')
    final_data = header_data + combined_data_length + combined_data

    with open(out_file, 'wb') as output_bin:
        output_bin.write(final_data)

    # Buat berkas header.
    current_year = datetime.now().year
    asset_name = os.path.basename(assets_path)
    header_file_path = os.path.join(include_path, f'mmap_generate_{asset_name}.h')
    with open(header_file_path, 'w', encoding='utf-8') as output_header:
        output_header.write('/*\n')
        output_header.write(' * SPDX-FileCopyrightText: 2022-{} Espressif Systems (Shanghai) CO LTD\n'.format(current_year))
        output_header.write(' *\n')
        output_header.write(' * SPDX-License-Identifier: Apache-2.0\n')
        output_header.write(' */\n\n')
        output_header.write('/**\n')
        output_header.write(' * @file\n')
        output_header.write(" * @brief Berkas ini dibuat otomatis oleh esp_mmap_assets, jangan diubah manual\n")
        output_header.write(' */\n\n')
        output_header.write('#pragma once\n\n')
        output_header.write("#include \"esp_mmap_assets.h\"\n\n")
        output_header.write(f'#define MMAP_{asset_name.upper()}_FILES           {total_files}\n')
        output_header.write(f'#define MMAP_{asset_name.upper()}_CHECKSUM        0x{combined_checksum:04X}\n\n')
        output_header.write(f'enum MMAP_{asset_name.upper()}_LISTS {{\n')

        for i, (file_name, _, _, _, _) in enumerate(file_info_list):
            enum_name = file_name.replace('.', '_')
            output_header.write(f'    MMAP_{asset_name.upper()}_{enum_name.upper()} = {i},        /*!< {file_name} */\n')

        output_header.write('};\n')

    print(f'Semua berkas sudah digabung ke {os.path.basename(out_file)}')


# =============================================================================
# Fungsi konfigurasi dan fungsi utama
# =============================================================================

def read_wakenet_from_sdkconfig(sdkconfig_path):
    """
    Baca model wakenet dari sdkconfig berdasarkan logika movemodel.py.

    Mengembalikan daftar nama model wakenet.
    """
    if not os.path.exists(sdkconfig_path):
        print(f"Warning: sdkconfig file not found: {sdkconfig_path}")
        return []
        
    models = []
    with io.open(sdkconfig_path, "r", encoding="utf-8") as f:
        for label in f:
            label = label.strip("\n")
            if 'CONFIG_SR_WN' in label and '#' not in label[0]:
                if '_NONE' in label:
                    continue
                if '=' in label:
                    label = label.split("=")[0]
                if '_MULTI' in label:
                    label = label[:-6]
                model_name = label.split("_SR_WN_")[-1].lower()
                models.append(model_name)

    return models


def read_multinet_from_sdkconfig(sdkconfig_path):
    """
    Baca model multinet dari sdkconfig berdasarkan logika movemodel.py.

    Mengembalikan daftar nama model multinet.
    """
    if not os.path.exists(sdkconfig_path):
        print(f"Warning: sdkconfig file not found: {sdkconfig_path}")
        return []
        
    with io.open(sdkconfig_path, "r", encoding="utf-8") as f:
        models_string = ''
        for label in f:
            label = label.strip("\n")
            if 'CONFIG_SR_MN' in label and label[0] != '#':
                models_string += label

    models = []
    if "CONFIG_SR_MN_CN_MULTINET3_SINGLE_RECOGNITION" in models_string:
        models.append('mn3_cn')
    elif "CONFIG_SR_MN_CN_MULTINET4_5_SINGLE_RECOGNITION_QUANT8" in models_string:
        models.append('mn4q8_cn')
    elif "CONFIG_SR_MN_CN_MULTINET4_5_SINGLE_RECOGNITION" in models_string:
        models.append('mn4_cn')
    elif "CONFIG_SR_MN_CN_MULTINET5_RECOGNITION_QUANT8" in models_string:
        models.append('mn5q8_cn')
    elif "CONFIG_SR_MN_CN_MULTINET6_QUANT" in models_string:
        models.append('mn6_cn')
    elif "CONFIG_SR_MN_CN_MULTINET6_AC_QUANT" in models_string:
        models.append('mn6_cn_ac')
    elif "CONFIG_SR_MN_CN_MULTINET7_QUANT" in models_string:
        models.append('mn7_cn')
    elif "CONFIG_SR_MN_CN_MULTINET7_AC_QUANT" in models_string:
        models.append('mn7_cn_ac')

    if "CONFIG_SR_MN_EN_MULTINET5_SINGLE_RECOGNITION_QUANT8" in models_string:
        models.append('mn5q8_en')
    elif "CONFIG_SR_MN_EN_MULTINET5_SINGLE_RECOGNITION" in models_string:
        models.append('mn5_en')
    elif "CONFIG_SR_MN_EN_MULTINET6_QUANT" in models_string:
        models.append('mn6_en')
    elif "CONFIG_SR_MN_EN_MULTINET7_QUANT" in models_string:
        models.append('mn7_en')

    if "MULTINET6" in models_string or "MULTINET7" in models_string:
        models.append('fst')

    return models


def read_wake_word_type_from_sdkconfig(sdkconfig_path):
    """
    Baca konfigurasi jenis wake word dari sdkconfig.

    Mengembalikan kamus yang berisi informasi jenis wake word.
    """
    if not os.path.exists(sdkconfig_path):
        print(f"Warning: sdkconfig file not found: {sdkconfig_path}")
        return {
            'use_esp_wake_word': False,
            'use_afe_wake_word': False,
            'use_custom_wake_word': False,
            'wake_word_disabled': True
        }
        
    config_values = {
        'use_esp_wake_word': False,
        'use_afe_wake_word': False,
        'use_custom_wake_word': False,
        'wake_word_disabled': False
    }
    
    with io.open(sdkconfig_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip("\n")
            if line.startswith('#'):
                continue
                
            # Periksa konfigurasi jenis wake word.
            if 'CONFIG_USE_ESP_WAKE_WORD=y' in line:
                config_values['use_esp_wake_word'] = True
            elif 'CONFIG_USE_AFE_WAKE_WORD=y' in line:
                config_values['use_afe_wake_word'] = True
            elif 'CONFIG_USE_CUSTOM_WAKE_WORD=y' in line:
                config_values['use_custom_wake_word'] = True
            elif 'CONFIG_WAKE_WORD_DISABLED=y' in line:
                config_values['wake_word_disabled'] = True
    
    return config_values


def read_custom_wake_word_from_sdkconfig(sdkconfig_path):
    """
    Baca konfigurasi wake word kustom dari sdkconfig.

    Mengembalikan kamus informasi wake word kustom, atau None jika belum diatur.
    """
    if not os.path.exists(sdkconfig_path):
        print(f"Warning: sdkconfig file not found: {sdkconfig_path}")
        return None
        
    config_values = {}
    with io.open(sdkconfig_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip("\n")
            if line.startswith('#') or '=' not in line:
                continue
                
            # Periksa konfigurasi wake word kustom.
            if 'CONFIG_USE_CUSTOM_WAKE_WORD=y' in line:
                config_values['use_custom_wake_word'] = True
            elif 'CONFIG_CUSTOM_WAKE_WORD=' in line and not line.startswith('#'):
                # Ambil nilai string tanpa tanda kutip.
                value = line.split('=', 1)[1].strip('"')
                config_values['wake_word'] = value
            elif 'CONFIG_CUSTOM_WAKE_WORD_DISPLAY=' in line and not line.startswith('#'):
                # Ambil nilai string tanpa tanda kutip.
                value = line.split('=', 1)[1].strip('"')
                config_values['display'] = value
            elif 'CONFIG_CUSTOM_WAKE_WORD_THRESHOLD=' in line and not line.startswith('#'):
                # Ambil nilai numerik.
                value = line.split('=', 1)[1]
                try:
                    config_values['threshold'] = int(value)
                except ValueError:
                    try:
                        config_values['threshold'] = float(value)
                    except ValueError:
                        print(f"Warning: Invalid threshold value: {value}")
                        config_values['threshold'] = 20  # nilai bawaan, nantinya dikonversi menjadi 0.2
    
    # Kembalikan konfigurasi hanya jika wake word kustom aktif dan field wajib lengkap.
    if (config_values.get('use_custom_wake_word', False) and 
        'wake_word' in config_values and 
        'display' in config_values and 
        'threshold' in config_values):
        return {
            'wake_word': config_values['wake_word'],
            'display': config_values['display'],
            'threshold': config_values['threshold'] / 100.0  # Ubah ke bentuk desimal, misalnya 20 menjadi 0.2.
        }
    
    return None


def get_language_from_multinet_models(multinet_models):
    """
    Tentukan bahasa berdasarkan nama model multinet.

    Mengembalikan `cn`, `en`, atau None.
    """
    if not multinet_models:
        return None
    
    # Cek apakah ada model bahasa Tionghoa.
    cn_indicators = ['_cn', 'cn_']
    en_indicators = ['_en', 'en_']
    
    has_cn = any(any(indicator in model for indicator in cn_indicators) for model in multinet_models)
    has_en = any(any(indicator in model for indicator in en_indicators) for model in multinet_models)
    
    # Jika keduanya ada atau tidak ada sama sekali, pakai nilai bawaan cn.
    if has_cn and not has_en:
        return 'cn'
    elif has_en and not has_cn:
        return 'en'
    else:
        return 'cn'  # Gunakan bahasa Tionghoa sebagai nilai bawaan.


def get_wakenet_model_paths(model_names, esp_sr_model_path):
    """
    Ambil path lengkap ke direktori model wakenet.

    Mengembalikan daftar path model yang valid.
    """
    if not model_names:
        return []
    
    valid_paths = []
    for model_name in model_names:
        wakenet_model_path = os.path.join(esp_sr_model_path, 'wakenet_model', model_name)
        if os.path.exists(wakenet_model_path):
            valid_paths.append(wakenet_model_path)
        else:
            print(f"Warning: Wakenet model directory not found: {wakenet_model_path}")
    
    return valid_paths


def get_multinet_model_paths(model_names, esp_sr_model_path):
    """
    Ambil path lengkap ke direktori model multinet.

    Mengembalikan daftar path model yang valid.
    """
    if not model_names:
        return []
    
    valid_paths = []
    for model_name in model_names:
        multinet_model_path = os.path.join(esp_sr_model_path, 'multinet_model', model_name)
        if os.path.exists(multinet_model_path):
            valid_paths.append(multinet_model_path)
        else:
            print(f"Warning: Multinet model directory not found: {multinet_model_path}")
    
    return valid_paths


def get_text_font_path(builtin_text_font, xiaozhi_fonts_path):
    """
    Ambil path font teks jika diperlukan.

    Mengembalikan path berkas font, atau None jika font tidak dibutuhkan.
    """
    if not builtin_text_font or 'basic' not in builtin_text_font:
        return None
    
    # Ubah nama font dari varian basic ke nama font common.
    # Contoh: font_puhui_basic_16_4 menjadi font_puhui_common_16_4.bin
    if builtin_text_font.startswith('font_noto_'):
        font_name = builtin_text_font.replace('basic', 'qwen') + '.bin'
    else:
        font_name = builtin_text_font.replace('basic', 'common') + '.bin'
    font_path = os.path.join(xiaozhi_fonts_path, 'cbin', font_name)
    
    if os.path.exists(font_path):
        return font_path
    else:
        print(f"Warning: Font file not found: {font_path}")
        return None


def get_emoji_collection_path(default_emoji_collection, xiaozhi_fonts_path, project_root=None):
    """
    Ambil path koleksi emoji jika diperlukan.

    Mengembalikan path direktori emoji, atau None jika tidak dibutuhkan.

    Yang didukung:
    - koleksi emoji PNG dari xiaozhi-fonts seperti `emojis_32` atau `twemoji_64`
    - koleksi emoji GIF dari xiaozhi-fonts seperti `noto-emoji_128`
    - koleksi Otto GIF `otto-gif`
    """
    if not default_emoji_collection:
        return None
    
    # Penanganan khusus untuk koleksi otto-gif.
    if default_emoji_collection == 'otto-gif':
        if project_root:
            otto_gif_path = os.path.join(project_root, 'managed_components', 
                                        'txp666__otto-emoji-gif-component', 'gifs')
            if os.path.exists(otto_gif_path):
                return otto_gif_path
            else:
                print(f"Warning: Otto GIF emoji collection directory not found: {otto_gif_path}")
                return None
        else:
            print("Warning: project_root not provided, cannot locate otto-gif collection")
            return None
    
    # Coba dulu koleksi emoji PNG.
    emoji_path = os.path.join(xiaozhi_fonts_path, 'png', default_emoji_collection)
    if os.path.exists(emoji_path):
        return emoji_path
    
    # Jika tidak ada, coba koleksi emoji GIF.
    emoji_path = os.path.join(xiaozhi_fonts_path, 'gif', default_emoji_collection)
    if os.path.exists(emoji_path):
        return emoji_path
    
    print(f"Warning: Emoji collection directory not found in png/ or gif/: {default_emoji_collection}")
    return None


def build_assets_integrated(wakenet_model_paths, multinet_model_paths, text_font_path, emoji_collection_path, extra_files_path, output_path, multinet_model_info=None):
    """
    Bangun aset dengan fungsi terintegrasi tanpa dependensi eksternal tambahan.
    """
    # Buat direktori build sementara
    temp_build_dir = os.path.join(os.path.dirname(output_path), "temp_build")
    assets_dir = os.path.join(temp_build_dir, "assets")
    
    try:
        # Bersihkan lalu buat ulang direktori kerja.
        if os.path.exists(temp_build_dir):
            remove_tree_with_retry(temp_build_dir, required=True)
        ensure_dir(temp_build_dir)
        ensure_dir(assets_dir)
        
        print("Starting to build assets...")
        
        # Proses tiap komponen aset.
        srmodels = process_sr_models(wakenet_model_paths, multinet_model_paths, temp_build_dir, assets_dir) if (wakenet_model_paths or multinet_model_paths) else None
        text_font = process_text_font(text_font_path, assets_dir) if text_font_path else None
        emoji_collection = process_emoji_collection(emoji_collection_path, assets_dir) if emoji_collection_path else None
        extra_files = process_extra_files(extra_files_path, assets_dir) if extra_files_path else None
        
        # Buat index.json.
        generate_index_json(assets_dir, srmodels, text_font, emoji_collection, extra_files, multinet_model_info)
        
        # Buat config.json untuk proses pengemasan.
        config_path = generate_config_json(temp_build_dir, assets_dir)
        
        # Muat konfigurasi lalu kemas aset.
        with open(config_path, 'r', encoding='utf-8') as f:
            config_data = json.load(f)
        
        # Gunakan fungsi pengemasan ringkas.
        include_path = config_data['include_path']
        image_file = config_data['image_file']
        pack_assets_simple(assets_dir, include_path, image_file, "assets", int(config_data['name_length']))
        
        # Salin assets.bin akhir ke lokasi keluaran.
        if os.path.exists(image_file):
            shutil.copy2(image_file, output_path)
            print(f"Successfully generated assets.bin: {output_path}")
            
            # Tampilkan informasi ukuran berkas.
            total_size = os.path.getsize(output_path)
            print(f"Assets file size: {total_size / 1024:.2f}K ({total_size} bytes)")
            
            return True
        else:
            print(f"Error: Generated assets.bin not found: {image_file}")
            return False
            
    except Exception as e:
        print(f"Error: Failed to build assets: {e}")
        return False
    finally:
        # Bersihkan direktori sementara.
        if os.path.exists(temp_build_dir):
            remove_tree_with_retry(temp_build_dir, required=False)


def main():
    parser = argparse.ArgumentParser(description='Build default assets based on configuration')
    parser.add_argument('--sdkconfig', required=True, help='Path to sdkconfig file')
    parser.add_argument('--builtin_text_font', help='Builtin text font name (e.g., font_puhui_basic_16_4)')
    parser.add_argument('--emoji_collection', help='Default emoji collection name (e.g., emojis_32)')
    parser.add_argument('--output', required=True, help='Output path for assets.bin')
    parser.add_argument('--esp_sr_model_path', help='Path to ESP-SR model directory')
    parser.add_argument('--xiaozhi_fonts_path', help='Path to xiaozhi-fonts component directory')
    parser.add_argument('--extra_files', help='Path to extra files directory to be included in assets')
    
    args = parser.parse_args()
    
    # Tetapkan path bawaan jika belum diberikan.
    if not args.esp_sr_model_path or not args.xiaozhi_fonts_path:
        # Hitung akar proyek dari lokasi skrip.
        script_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(script_dir)
        
        if not args.esp_sr_model_path:
            args.esp_sr_model_path = os.path.join(project_root, "managed_components", "espressif__esp-sr", "model")
        
        if not args.xiaozhi_fonts_path:
            args.xiaozhi_fonts_path = os.path.join(project_root, "components", "xiaozhi-fonts")
    
    print("Building default assets...")
    print(f"  sdkconfig: {args.sdkconfig}")
    print(f"  builtin_text_font: {args.builtin_text_font}")
    print(f"  emoji_collection: {args.emoji_collection}")
    print(f"  output: {args.output}")
    
    # Baca konfigurasi jenis wake word dari sdkconfig.
    wake_word_config = read_wake_word_type_from_sdkconfig(args.sdkconfig)
    
    # Baca model SR dari sdkconfig.
    wakenet_model_names = read_wakenet_from_sdkconfig(args.sdkconfig)
    multinet_model_names = read_multinet_from_sdkconfig(args.sdkconfig)
    
    # Terapkan logika wake word untuk menentukan model yang perlu dikemas.
    wakenet_model_paths = []
    multinet_model_paths = []
    
    # 1. Kemas model wakenet hanya jika USE_ESP_WAKE_WORD=y atau USE_AFE_WAKE_WORD=y.
    if wake_word_config['use_esp_wake_word'] or wake_word_config['use_afe_wake_word']:
        wakenet_model_paths = get_wakenet_model_paths(wakenet_model_names, args.esp_sr_model_path)
    elif wakenet_model_names:
        print(f"  Note: Found wakenet models {wakenet_model_names} but wake word type is not ESP/AFE, skipping")
    
    # 2. Cek error: jika USE_CUSTOM_WAKE_WORD=y tetapi tidak ada multinet, laporkan error.
    if wake_word_config['use_custom_wake_word'] and not multinet_model_names:
        print("Error: USE_CUSTOM_WAKE_WORD is enabled but no multinet models are selected in sdkconfig")
        print("Please select appropriate CONFIG_SR_MN_* options in menuconfig, or disable USE_CUSTOM_WAKE_WORD")
        sys.exit(1)
    
    # 3. Kemas model multinet hanya jika USE_CUSTOM_WAKE_WORD=y.
    if wake_word_config['use_custom_wake_word']:
        multinet_model_paths = get_multinet_model_paths(multinet_model_names, args.esp_sr_model_path)
    elif multinet_model_names:
        print(f"  Note: Found multinet models {multinet_model_names} but USE_CUSTOM_WAKE_WORD is disabled, skipping")
    
    # Tampilkan informasi model yang benar-benar akan dikemas.
    if wakenet_model_paths:
        print(f"  wakenet models: {', '.join(wakenet_model_names)} (will be packaged)")
    if multinet_model_paths:
        print(f"  multinet models: {', '.join(multinet_model_names)} (will be packaged)")
    
    # Ambil path font teks jika dibutuhkan.
    text_font_path = get_text_font_path(args.builtin_text_font, args.xiaozhi_fonts_path)
    
    # Ambil path koleksi emoji jika dibutuhkan.
    # Hitung akar proyek dari lokasi skrip untuk mendukung otto-gif.
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    emoji_collection_path = get_emoji_collection_path(args.emoji_collection, args.xiaozhi_fonts_path, project_root)
    
    # Ambil path berkas tambahan jika disediakan.
    extra_files_path = args.extra_files
    
    # Baca konfigurasi wake word kustom.
    custom_wake_word_config = read_custom_wake_word_from_sdkconfig(args.sdkconfig)
    multinet_model_info = None
    
    if custom_wake_word_config and multinet_model_paths:
        # Tentukan bahasa dari model multinet.
        language = get_language_from_multinet_models(multinet_model_names)
        
        # Susun struktur informasi multinet_model.
        multinet_model_info = {
            "language": language,
            "duration": 3000,  # Durasi bawaan dalam milidetik.
            "threshold": custom_wake_word_config['threshold'],
            "commands": [
                {
                    "command": custom_wake_word_config['wake_word'],
                    "text": custom_wake_word_config['display'],
                    "action": "wake"
                }
            ]
        }
        print(f"  custom wake word: {custom_wake_word_config['wake_word']} ({custom_wake_word_config['display']})")
        print(f"  wake word language: {language}")
        print(f"  wake word threshold: {custom_wake_word_config['threshold']}")
    
    # Pastikan memang ada aset yang perlu dibangun.
    if not wakenet_model_paths and not multinet_model_paths and not text_font_path and not emoji_collection_path and not extra_files_path and not multinet_model_info:
        print("Warning: No assets to build (no SR models, text font, emoji collection, extra files, or custom wake word)")
        # Buat assets.bin kosong.
        os.makedirs(os.path.dirname(args.output), exist_ok=True)
        with open(args.output, 'wb') as f:
            pass  # Buat berkas kosong
        print(f"Created empty assets.bin: {args.output}")
        return
    
    # Bangun seluruh aset.
    success = build_assets_integrated(wakenet_model_paths, multinet_model_paths, text_font_path, emoji_collection_path, 
                                     extra_files_path, args.output, multinet_model_info)
    
    if not success:
        sys.exit(1)
    
    print("Build completed successfully!")


if __name__ == "__main__":
    main()
