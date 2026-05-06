#!/usr/bin/env python3
"""Ekspor CAMPUS_DB dari C++ ke JSON canonical untuk server RAG."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from campus_rag_server import DEFAULT_JSON, DEFAULT_SOURCE, parse_campus_source


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", default=str(DEFAULT_SOURCE))
    parser.add_argument("--output", default=str(DEFAULT_JSON))
    args = parser.parse_args()

    entries = parse_campus_source(Path(args.source))
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(entries, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Berhasil menulis {len(entries)} entri ke {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
