#!/usr/bin/env python3
"""Audit retrieval server RAG kampus tanpa perlu menyalakan HTTP server."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from campus_rag_server import CampusRag, DEFAULT_JSON, DEFAULT_SOURCE, load_entries, normalize_text


CHECKS = [
    ("Siapa rektor UCIC?", ["chandra lukita"]),
    ("rektor kampus siapa?", ["chandra lukita"]),
    ("Siapa dosen agama?", ["amroni", "feri fauzi", "petrus sokibi"]),
    ("Siapa yang ngajar agama Kristen?", ["petrus sokibi"]),
    ("Siapa dosen algoritma pemrograman?", ["marsani asfi", "kusnadi", "petrus sokibi"]),
    ("Siapa dosen algoritma pemrograman 2?", ["marsani asfi", "muhammad hatta"]),
    ("Kenal dosen Petrus Sokibi?", ["petrus sokibi"]),
    ("Marsani Asfi ngajar apa?", ["marsani asfi", "algoritma"]),
    ("Akreditasi Teknik Informatika", ["baik sekali"]),
    ("Biaya D3 Manajemen Informatika opsi 2", ["d3 manajemen informatika", "total"]),
    ("Jurusan UCIC apa saja?", ["teknik informatika", "sistem informasi", "bisnis digital"]),
]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--data", default=str(DEFAULT_JSON))
    parser.add_argument("--source", default=str(DEFAULT_SOURCE))
    parser.add_argument("--embedding-model", default="")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()

    rag = CampusRag(load_entries(Path(args.data), Path(args.source)), embedding_model=args.embedding_model)
    failures: list[dict[str, object]] = []
    results: list[dict[str, object]] = []

    for query, expected_terms in CHECKS:
        payload = rag.answer(query)
        answer_norm = normalize_text(payload.get("answer", ""))
        missing = [term for term in expected_terms if normalize_text(term) not in answer_norm]
        row = {
            "query": query,
            "status": payload.get("status"),
            "confidence": payload.get("confidence"),
            "answer": payload.get("answer"),
            "missing": missing,
        }
        results.append(row)
        if payload.get("status") != "found" or missing:
            failures.append(row)

    report = {
        "entries": len(rag.entries),
        "chunks": len(rag.chunks),
        "checks": len(CHECKS),
        "failures": failures,
        "results": results,
    }

    if args.json:
        print(json.dumps(report, ensure_ascii=False, indent=2))
    else:
        print(f"Entri: {report['entries']}")
        print(f"Chunk: {report['chunks']}")
        print(f"Uji: {report['checks']}")
        print(f"Gagal: {len(failures)}")
        for failure in failures:
            print(json.dumps(failure, ensure_ascii=False))

    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
