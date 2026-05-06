import sys
import os
import json
import zipfile
import argparse
from pathlib import Path
from typing import Optional

# Pindah ke akar proyek agar semua perintah menggunakan jalur yang konsisten.
os.chdir(Path(__file__).resolve().parent.parent)

################################################################################
# Fungsi utilitas umum
################################################################################


def get_board_type_from_compile_commands() -> Optional[str]:
    """Baca BOARD_TYPE hasil kompilasi saat ini dari build/compile_commands.json."""
    compile_file = Path("build/compile_commands.json")
    if not compile_file.exists():
        return None
    with compile_file.open(encoding='utf-8') as f:
        data = json.load(f)
    for item in data:
        if not item["file"].endswith("main.cc"):
            continue
        cmd = item["command"]
        if "-DBOARD_TYPE=\\\"" in cmd:
            return cmd.split("-DBOARD_TYPE=\\\"")[1].split("\\\"")[0].strip()
    return None


def get_project_version() -> Optional[str]:
    """Baca set(PROJECT_VER "x.y.z") dari CMakeLists.txt akar proyek."""
    with Path("CMakeLists.txt").open(encoding='utf-8') as f:
        for line in f:
            if line.startswith("set(PROJECT_VER"):
                return line.split("\"")[1]
    return None


def merge_bin() -> None:
    if os.system("idf.py merge-bin") != 0:
        print("merge-bin gagal", file=sys.stderr)
        sys.exit(1)


def zip_bin(name: str, version: str) -> None:
    """Kemas build/merged-binary.bin ke releases/v{version}_{name}.zip."""
    out_dir = Path("releases")
    out_dir.mkdir(exist_ok=True)
    output_path = out_dir / f"v{version}_{name}.zip"

    if output_path.exists():
        output_path.unlink()

    with zipfile.ZipFile(output_path, "w", compression=zipfile.ZIP_DEFLATED) as zipf:
        zipf.write("build/merged-binary.bin", arcname="merged-binary.bin")
    print(f"berhasil membuat arsip biner di {output_path}")


def _get_manufacturer(cfg: dict) -> Optional[str]:
    """Baca nama pabrikan dari config.json."""
    manufacturer = cfg.get("manufacturer")
    if isinstance(manufacturer, str) and manufacturer.strip():
        return manufacturer.strip()
    return None


################################################################################
# Fungsi terkait board dan varian
################################################################################

_BOARDS_DIR = Path("main/boards")


def _collect_variants(config_filename: str = "config.json") -> list[dict[str, str]]:
    """
    Telusuri seluruh board di main/boards lalu kumpulkan informasi variannya.

    Contoh hasil:
        [{"board": "bread-compact-ml307", "name": "bread-compact-ml307", "full_name": "bread-compact-ml307"}]
        [{"board": "waveshare/esp32-p4-nano", "name": "esp32-p4-nano-10.1-a", "full_name": "waveshare-esp32-p4-nano-10.1-a"}]
    """
    variants: list[dict[str, str]] = []
    errors: list[str] = []

    for cfg_path in _BOARDS_DIR.rglob(config_filename):
        board_dir = cfg_path.parent
        if board_dir.name == "common":
            continue
        board = board_dir.relative_to(_BOARDS_DIR).as_posix()

        try:
            with cfg_path.open(encoding='utf-8') as f:
                cfg = json.load(f)

            manufacturer = _get_manufacturer(cfg)

            # Pastikan nilai manufacturer konsisten dengan struktur direktori.
            if "/" in board:
                expected_manufacturer = board.split("/")[0]
                if not manufacturer:
                    errors.append(
                        f"{cfg_path}: board berada di subdirektori '{expected_manufacturer}/', "
                        f"tetapi config.json tidak memiliki \"manufacturer\": \"{expected_manufacturer}\""
                    )
                elif manufacturer != expected_manufacturer:
                    errors.append(
                        f"{cfg_path}: nilai manufacturer tidak cocok, "
                        f"direktori memakai '{expected_manufacturer}' tetapi config.json berisi \"{manufacturer}\""
                    )
            else:
                if manufacturer:
                    errors.append(
                        f"{cfg_path}: board tidak berada di subdirektori pabrikan, "
                        f"tetapi config.json mendefinisikan manufacturer \"{manufacturer}\", "
                        f"silakan pindahkan board ke main/boards/{manufacturer}/{board}/"
                    )

            for build in cfg.get("builds", []):
                name = build["name"]
                full_name = f"{manufacturer}-{name}" if manufacturer else name
                variants.append({
                    "board": board,
                    "name": name,
                    "full_name": full_name
                })

        except Exception as e:
            print(f"[ERROR] gagal mem-parsing {cfg_path}: {e}", file=sys.stderr)

    # Laporkan semua masalah sekaligus agar lebih mudah diperbaiki.
    if errors:
        print("\n[ERROR] ditemukan masalah konfigurasi manufacturer:", file=sys.stderr)
        for err in errors:
            print(f"  - {err}", file=sys.stderr)
        print(file=sys.stderr)
        sys.exit(1)

    return variants


def _find_board_config(board_type: str) -> Optional[str]:
    """
    Cari CONFIG_BOARD_TYPE_xxx yang sesuai untuk board_type tertentu.

    Pencarian dilakukan mundur dari baris `set(BOARD_TYPE "xxx")` menuju
    blok `if(CONFIG_BOARD_TYPE_...)` terdekat.
    """
    board_leaf = board_type.split("/")[-1]
    pattern = f'set(BOARD_TYPE "{board_leaf}")'

    cmake_file = Path("main/CMakeLists.txt")
    lines = cmake_file.read_text(encoding="utf-8").splitlines()

    for idx, line in enumerate(lines):
        if pattern in line:
            for back_idx in range(idx - 1, -1, -1):
                back_line = lines[back_idx]
                if "if(CONFIG_BOARD_TYPE_" in back_line:
                    return back_line.strip().split("if(")[1].split(")")[0]
            break
    return None


# Entri "select" pada Kconfig tidak otomatis diterapkan saat kita hanya
# menambahkan baris ke sdkconfig, jadi dependensi wajibnya kita tambahkan di sini
# agar perilakunya mendekati menuconfig.
_AUTO_SELECT_RULES: dict[str, list[str]] = {
    "CONFIG_USE_ESP_BLUFI_WIFI_PROVISIONING": [
        "CONFIG_BT_ENABLED=y",
        "CONFIG_BT_BLUEDROID_ENABLED=y",
        "CONFIG_BT_BLE_42_FEATURES_SUPPORTED=y",
        "CONFIG_BT_BLE_50_FEATURES_SUPPORTED=n",
        "CONFIG_BT_BLE_BLUFI_ENABLE=y",
        "CONFIG_MBEDTLS_DHM_C=y",
    ],
}


def _apply_auto_selects(sdkconfig_append: list[str]) -> list[str]:
    """Terapkan aturan auto-select yang dipetakan secara statis."""
    items: list[str] = []
    existing_keys: set[str] = set()

    def _append_if_missing(entry: str) -> None:
        key = entry.split("=", 1)[0]
        if key not in existing_keys:
            items.append(entry)
            existing_keys.add(key)

    # Pertahankan urutan asli sambil melacak key yang sudah ditambahkan.
    for entry in sdkconfig_append:
        _append_if_missing(entry)

    # Tambahkan dependensi otomatis saat fitur utama diaktifkan.
    for key, deps in _AUTO_SELECT_RULES.items():
        for entry in sdkconfig_append:
            name, _, value = entry.partition("=")
            if name == key and value.lower().startswith("y"):
                for dep in deps:
                    _append_if_missing(dep)
                break

    return items


################################################################################
# Pemeriksaan board_type di CMakeLists
################################################################################


def _board_type_exists(board_type: str) -> bool:
    cmake_file = Path("main/CMakeLists.txt").read_text(encoding="utf-8")
    board_leaf = board_type.split("/")[-1]
    pattern = f'set(BOARD_TYPE "{board_leaf}")'
    return pattern in cmake_file


################################################################################
# Implementasi kompilasi
################################################################################


def release(board_type: str, config_filename: str = "config.json", *, filter_name: Optional[str] = None) -> None:
    """
    Kompilasi dan kemas semua varian, atau satu varian tertentu, untuk board_type.

    Argumen:
        board_type: nama direktori di bawah main/boards.
        config_filename: nama file config.json yang dipakai.
        filter_name: jika diisi, hanya build["name"] yang sama yang akan dikompilasi.
    """
    cfg_path = _BOARDS_DIR / Path(board_type) / config_filename
    if not cfg_path.exists():
        print(f"[WARN] {cfg_path} tidak ada, melewati {board_type}")
        return

    project_version = get_project_version()
    print(f"Versi proyek: {project_version} ({cfg_path})")

    with cfg_path.open(encoding='utf-8') as f:
        cfg = json.load(f)
    target = cfg["target"]
    manufacturer = _get_manufacturer(cfg)

    builds = cfg.get("builds", [])
    if filter_name:
        builds = [b for b in builds if b["name"] == filter_name]
        if not builds:
            print(f"[ERROR] varian {filter_name} tidak ditemukan pada {board_type} di {config_filename}", file=sys.stderr)
            sys.exit(1)

    for build in builds:
        name = build["name"]
        board_leaf = board_type.split("/")[-1]

        if board_leaf not in name:
            raise ValueError(f"build.name {name} harus memuat {board_leaf}")

        final_name = f"{manufacturer}-{name}" if manufacturer else name
        output_path = Path("releases") / f"v{project_version}_{final_name}.zip"
        if output_path.exists():
            print(f"Melewati {final_name} karena {output_path} sudah ada")
            continue

        # Proses daftar tambahan sdkconfig.
        board_type_config = _find_board_config(board_type)
        sdkconfig_append = [f"{board_type_config}=y"]
        sdkconfig_append.extend(build.get("sdkconfig_append", []))
        sdkconfig_append = _apply_auto_selects(sdkconfig_append)

        print("-" * 80)
        print(f"nama: {final_name}")
        print(f"target: {target}")
        if manufacturer:
            print(f"manufacturer: {manufacturer}")
        for item in sdkconfig_append:
            print(f"sdkconfig_append: {item}")

        os.environ.pop("IDF_TARGET", None)

        if os.system(f"idf.py set-target {target}") != 0:
            print("set-target gagal", file=sys.stderr)
            sys.exit(1)

        # Tambahkan konfigurasi board ke sdkconfig.
        with Path("sdkconfig").open("a", encoding='utf-8') as f:
            f.write("\n")
            f.write("# Ditambahkan oleh release.py\n")
            for append in sdkconfig_append:
                f.write(f"{append}\n")

        # Bangun firmware dengan BOARD_NAME dan BOARD_TYPE yang sesuai.
        if os.system(f"idf.py -DBOARD_NAME={name} -DBOARD_TYPE={board_type} build") != 0:
            print("build gagal")
            sys.exit(1)

        merge_bin()
        zip_bin(final_name, project_version)


################################################################################
# Titik masuk CLI
################################################################################

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("board", nargs="?", default=None, help="Jenis board atau 'all'")
    parser.add_argument("-c", "--config", default="config.json", help="Nama file config (bawaan: config.json)")
    parser.add_argument("--list-boards", action="store_true", help="Tampilkan semua board dan variannya")
    parser.add_argument("--json", action="store_true", help="Keluarkan hasil JSON saat dipakai dengan --list-boards")
    parser.add_argument("--name", help="Nama varian yang akan dikompilasi tanpa awalan manufacturer")

    args = parser.parse_args()

    # Mode daftar board.
    if args.list_boards:
        variants = _collect_variants(config_filename=args.config)
        if args.json:
            print(json.dumps(variants))
        else:
            for variant in variants:
                print(f"{variant['board']}: {variant['name']}")
        sys.exit(0)

    # Mode pengemasan firmware dari hasil build direktori saat ini.
    if args.board is None:
        merge_bin()
        curr_board_type = get_board_type_from_compile_commands()
        if curr_board_type is None:
            print("gagal membaca board_type dari compile_commands.json", file=sys.stderr)
            sys.exit(1)
        project_ver = get_project_version()
        zip_bin(curr_board_type, project_ver)
        sys.exit(0)

    board_type_input: str = args.board
    name_filter: Optional[str] = args.name

    # Pastikan board_type benar-benar ada di CMakeLists.
    if board_type_input != "all" and not _board_type_exists(board_type_input):
        print(f"[ERROR] board_type {board_type_input} tidak ditemukan di main/CMakeLists.txt", file=sys.stderr)
        sys.exit(1)

    variants_all = _collect_variants(config_filename=args.config)

    # Siapkan daftar board_type yang akan diproses.
    target_board_types: set[str]
    if board_type_input == "all":
        target_board_types = {variant["board"] for variant in variants_all}
    else:
        target_board_types = {board_type_input}

    for board_type in sorted(target_board_types):
        if not _board_type_exists(board_type):
            print(f"[ERROR] board_type {board_type} tidak ditemukan di main/CMakeLists.txt", file=sys.stderr)
            sys.exit(1)
        cfg_path = _BOARDS_DIR / board_type / args.config
        if board_type == board_type_input and not cfg_path.exists():
            print(f"Board {board_type} tidak memiliki file {args.config}, dilewati")
            sys.exit(0)
        release(board_type, config_filename=args.config, filter_name=name_filter if board_type == board_type_input else None)
