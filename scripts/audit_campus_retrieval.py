#!/usr/bin/env python3
import argparse
import json
import re
import subprocess
import sys
from pathlib import Path


ENTRY_PATTERN = re.compile(
    r'\{\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}',
    re.DOTALL,
)

SUMMARY_IDS = {"dosen_fti", "dosen_feb", "dosen_fps", "dosen_mku"}


def decode_c_string(value: str) -> str:
    return bytes(value, "utf-8").decode("unicode_escape")


def normalize_text(text: str) -> str:
    text = text.lower()
    text = re.sub(r"[^a-z0-9]+", " ", text)
    return re.sub(r"\s+", " ", text).strip()


def parse_entries(source_path: Path) -> list[dict[str, str]]:
    source = source_path.read_text(encoding="utf-8")
    marker = "static const CampusEntry CAMPUS_DB[] = {"
    start = source.find(marker)
    if start < 0:
        raise RuntimeError("CAMPUS_DB marker not found")
    end = source.find("};", start)
    if end < 0:
        raise RuntimeError("CAMPUS_DB terminator not found")
    body = source[start:end]

    entries: list[dict[str, str]] = []
    for match in ENTRY_PATTERN.finditer(body):
        entries.append(
            {
                "id": decode_c_string(match.group(1)),
                "category": decode_c_string(match.group(2)),
                "title": decode_c_string(match.group(3)),
                "content": decode_c_string(match.group(4)),
                "keywords": decode_c_string(match.group(5)),
            }
        )
    return entries


def base_lecturer_name(title: str) -> str:
    title = title.split(",")[0].strip()
    title = re.sub(r"^(Assoc\. Prof\. Dr\.|Assoc\. Prof\.|Prof\.|Dr\.)\s*", "", title, flags=re.I)
    return title


def extract_lecturer_courses(content: str) -> list[str]:
    if ":" not in content:
        return []
    course_block = content.split(":", 1)[1].replace("\n", ",")
    return [piece.strip(" .") for piece in course_block.split(",") if piece.strip(" .")]


def run_query(cli_path: Path, query: str) -> str:
    return subprocess.check_output([str(cli_path), query], text=True, encoding="utf-8", errors="ignore")


def audit_lecturer_courses(entries: list[dict[str, str]], cli_path: Path) -> list[dict[str, str]]:
    failures: list[dict[str, str]] = []
    for entry in entries:
        if entry["category"] != "dosen" or entry["id"] in SUMMARY_IDS:
            continue

        lecturer_name = base_lecturer_name(entry["title"])
        normalized_name = normalize_text(lecturer_name)
        for course in extract_lecturer_courses(entry["content"]):
            query = f"siapa dosen {course}"
            response = normalize_text(run_query(cli_path, query))
            if normalized_name not in response:
                failures.append(
                    {
                        "id": entry["id"],
                        "lecturer": lecturer_name,
                        "course": course,
                        "query": query,
                    }
                )
    return failures


def audit_core_queries(cli_path: Path) -> list[dict[str, str]]:
    expected_checks = [
        ("Siapa dosen agama?", ["amroni", "feri fauzi", "petrus sokibi"]),
        ("Siapa dosen agama kristen?", ["petrus sokibi"]),
        ("Siapa dosen algoritma pemrograman?", ["marsani asfi", "kusnadi", "petrus sokibi"]),
        ("Siapa dosen algoritma pemrograman 2", ["marsani asfi", "muhammad hatta"]),
        ("Siapa rektor UCIC?", ["chandra lukita"]),
        ("Semester 1 Teknik Informatika belajar apa aja?", ["algoritma dan pemrograman"]),
        ("Akreditasi Teknik Informatika", ["baik sekali"]),
        ("Biaya Teknik Informatika opsi 1", ["total", "teknik informatika"]),
    ]

    failures: list[dict[str, str]] = []
    for query, expected_terms in expected_checks:
        response = normalize_text(run_query(cli_path, query))
        missing = [term for term in expected_terms if normalize_text(term) not in response]
        if missing:
            failures.append({"query": query, "missing": ", ".join(missing)})
    return failures


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", default="main/campus_data.cc")
    parser.add_argument("--cli", required=True, help="Path to host-side query executable")
    parser.add_argument("--json", action="store_true", help="Print JSON result")
    args = parser.parse_args()

    entries = parse_entries(Path(args.source))
    cli_path = Path(args.cli)

    lecturer_failures = audit_lecturer_courses(entries, cli_path)
    core_failures = audit_core_queries(cli_path)

    result = {
        "entry_count": len(entries),
        "lecturer_course_queries": sum(
            1
            for entry in entries
            if entry["category"] == "dosen" and entry["id"] not in SUMMARY_IDS
            for _ in extract_lecturer_courses(entry["content"])
        ),
        "lecturer_course_failures": lecturer_failures,
        "core_failures": core_failures,
    }

    if args.json:
        print(json.dumps(result, ensure_ascii=False, indent=2))
    else:
        print(f"Campus entries: {result['entry_count']}")
        print(f"Lecturer course queries: {result['lecturer_course_queries']}")
        print(f"Lecturer course failures: {len(lecturer_failures)}")
        print(f"Core query failures: {len(core_failures)}")
        if lecturer_failures:
            print("Sample lecturer course failures:")
            for item in lecturer_failures[:20]:
                print(json.dumps(item, ensure_ascii=False))
        if core_failures:
            print("Core failures:")
            for item in core_failures:
                print(json.dumps(item, ensure_ascii=False))

    return 1 if lecturer_failures or core_failures else 0


if __name__ == "__main__":
    sys.exit(main())
