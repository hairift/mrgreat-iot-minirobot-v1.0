#!/usr/bin/env python3
"""
Unduh artefak GitHub Actions lalu ubah namanya dengan nomor versi.

Contoh:
    python download_github_runs.py 2.0.4 https://github.com/78/xiaozhi-esp32/actions/runs/18866246016

Keluaran:
    Berkas akan diunduh ke direktori releases/<versi>/ relatif terhadap akar proyek.
    Contoh: releases/2.0.4/v2.0.4_atk-dnesp32s3-box0.zip
"""

import argparse
import os
import re
import sys
from pathlib import Path

import requests
from dotenv import load_dotenv


def parse_github_run_url(url: str) -> tuple[str, str, str]:
    """
    Uraikan URL GitHub Actions untuk mengambil owner, repo, dan run_id.

    Argumen:
        url: URL run GitHub Actions

    Nilai balik:
        Tuple berisi (owner, repo, run_id)
    """
    pattern = r"github\.com/([^/]+)/([^/]+)/actions/runs/(\d+)"
    match = re.search(pattern, url)

    if not match:
        raise ValueError(f"URL GitHub Actions tidak valid: {url}")

    owner, repo, run_id = match.groups()
    return owner, repo, run_id


def get_artifacts(owner: str, repo: str, run_id: str, token: str) -> list[dict]:
    """
    Ambil seluruh artefak untuk satu workflow run, termasuk halaman lanjutan.

    Argumen:
        owner: Pemilik repositori
        repo: Nama repositori
        run_id: ID workflow run
        token: Token akses GitHub

    Nilai balik:
        Daftar kamus artefak
    """
    headers = {
        "Authorization": f"Bearer {token}",
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
    }

    all_artifacts = []
    page = 1
    per_page = 100  # Batas maksimum yang diizinkan GitHub API

    while True:
        url = f"https://api.github.com/repos/{owner}/{repo}/actions/runs/{run_id}/artifacts"
        params = {
            "page": page,
            "per_page": per_page,
        }

        response = requests.get(url, headers=headers, params=params)
        response.raise_for_status()

        data = response.json()
        artifacts = data.get("artifacts", [])

        if not artifacts:
            break

        all_artifacts.extend(artifacts)

        # Periksa apakah masih ada halaman berikutnya
        total_count = data.get("total_count", 0)
        if len(all_artifacts) >= total_count:
            break

        page += 1

    return all_artifacts


def download_artifact(artifact_url: str, token: str, output_path: Path) -> None:
    """
    Unduh satu artefak dari GitHub.

    Argumen:
        artifact_url: URL unduhan artefak
        token: Token akses GitHub
        output_path: Lokasi penyimpanan artefak
    """
    headers = {
        "Authorization": f"Bearer {token}",
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
    }

    response = requests.get(artifact_url, headers=headers, stream=True)
    response.raise_for_status()

    # Pastikan direktori induk sudah ada
    output_path.parent.mkdir(parents=True, exist_ok=True)

    # Unduh berkas ke lokasi tujuan
    with open(output_path, "wb") as f:
        for chunk in response.iter_content(chunk_size=8192):
            if chunk:
                f.write(chunk)


def rename_artifact(original_name: str, version: str) -> str:
    """
    Ubah nama artefak sesuai aturan versi proyek.

    Aturan:
    - hapus awalan "xiaozhi_"
    - hapus akhiran hash
    - tambahkan awalan versi, misalnya "v2.0.4_"
    - tambahkan ekstensi .zip

    Argumen:
        original_name: Nama artefak asli
        version: Nomor versi, misalnya "2.0.4"

    Nilai balik:
        Nama berkas baru
    """
    name = original_name
    if name.startswith("xiaozhi_"):
        name = name[len("xiaozhi_"):]

    # Hapus ekstensi yang dikenal tanpa memakai splitext agar nama dengan titik tetap aman
    known_extensions = (".bin", ".zip")
    for ext in known_extensions:
        if name.endswith(ext):
            name = name[:-len(ext)]
            break

    # Hapus akhiran hash berupa underscore diikuti 40+ karakter heksadesimal
    name_without_hash = re.sub(r"_[a-f0-9]{40,}$", "", name)

    return f"v{version}_{name_without_hash}.zip"


def get_default_releases_dir() -> Path:
    """
    Ambil lokasi bawaan direktori releases relatif terhadap skrip ini.

    Nilai balik:
        Path ke direktori releases
    """
    script_dir = Path(__file__).resolve().parent
    return script_dir.parent / "releases"


def main():
    """Fungsi utama untuk mengunduh dan mengganti nama artefak GitHub Actions."""
    parser = argparse.ArgumentParser(
        description="Unduh artefak GitHub Actions lalu ubah namanya dengan nomor versi."
    )
    parser.add_argument(
        "version",
        help="Nomor versi, misalnya 2.0.4",
    )
    parser.add_argument(
        "url",
        help="URL run GitHub Actions, misalnya https://github.com/owner/repo/actions/runs/12345",
    )
    parser.add_argument(
        "--output-dir",
        default=None,
        help="Direktori keluaran untuk artefak unduhan, bawaan: releases/<versi> relatif ke akar proyek",
    )

    args = parser.parse_args()

    # Muat token GitHub dari berkas .env
    load_dotenv()
    github_token = os.getenv("GITHUB_TOKEN")

    if not github_token:
        print("Error: GITHUB_TOKEN tidak ditemukan di variabel lingkungan.", file=sys.stderr)
        print("Buat berkas .env yang berisi GITHUB_TOKEN=token_anda", file=sys.stderr)
        sys.exit(1)

    try:
        # Uraikan URL GitHub
        owner, repo, run_id = parse_github_run_url(args.url)
        print(f"Repositori: {owner}/{repo}")
        print(f"Run ID: {run_id}")
        print(f"Versi: {args.version}")
        print()

        # Ambil daftar artefak
        print("Mengambil daftar artefak...")
        artifacts = get_artifacts(owner, repo, run_id, github_token)

        if not artifacts:
            print("Tidak ada artefak untuk run ini.")
            return

        print(f"Ditemukan {len(artifacts)} artefak:")
        for artifact in artifacts:
            print(f"  - {artifact['name']}")
        print()

        # Tentukan direktori keluaran
        if args.output_dir:
            output_dir = Path(args.output_dir) / args.version
        else:
            output_dir = get_default_releases_dir() / args.version

        # Pastikan direktori keluaran tersedia
        output_dir.mkdir(parents=True, exist_ok=True)

        downloaded_count = 0
        skipped_count = 0

        # Unduh dan ubah nama setiap artefak
        for artifact in artifacts:
            original_name = artifact["name"]
            new_name = rename_artifact(original_name, args.version)
            final_path = output_dir / new_name

            # Lewati jika berkas final sudah ada
            if final_path.exists():
                print(f"Melewati karena sudah ada: {original_name}")
                print(f"  -> {new_name}")
                print(f"  Berkas: {final_path}")
                print()
                skipped_count += 1
                continue

            print(f"Mengunduh: {original_name}")
            print(f"  -> {new_name}")

            # Unduh dulu ke nama sementara
            temp_path = output_dir / f"{original_name}.zip"
            download_artifact(
                artifact["archive_download_url"],
                github_token,
                temp_path,
            )

            # Ganti ke nama akhir
            temp_path.rename(final_path)

            print(f"  Disimpan ke: {final_path}")
            print()
            downloaded_count += 1

        print("Ringkasan:")
        print(f"  Diunduh: {downloaded_count} artefak")
        print(f"  Dilewati: {skipped_count} artefak")
        print(f"  Total: {len(artifacts)} artefak")
        print(f"  Direktori keluaran: {output_dir.absolute()}")

    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
