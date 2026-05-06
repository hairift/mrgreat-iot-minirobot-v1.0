#!/usr/bin/env python3
"""
Server retrieval augmented generation ringan untuk data kampus UCIC.

Server ini sengaja ditempatkan di sisi komputer/server, bukan di ESP32, agar
embedding, FAISS, dan pencarian fuzzy tidak memakan flash atau RAM perangkat.
Jika dependensi AI belum terpasang, server tetap berjalan memakai indeks hybrid
berbasis token, fuzzy sederhana, dan vektor hash karakter.
"""

from __future__ import annotations

import argparse
import ast
import json
import math
import re
import socket
import sys
import time
from dataclasses import dataclass
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from difflib import SequenceMatcher
from typing import Any
from urllib.parse import parse_qs, unquote_plus, urlparse

try:
    from rapidfuzz import fuzz as rapid_fuzz
except Exception:
    rapid_fuzz = None

np = None
faiss = None
SentenceTransformer = None


def lazy_import_vector_dependencies(load_model: bool, load_faiss: bool) -> tuple[Any, Any, Any]:
    """Muat dependensi berat hanya saat benar-benar diminta."""
    global np, faiss, SentenceTransformer
    if (load_model or load_faiss) and np is None:
        try:
            import numpy as numpy_module
            np = numpy_module
        except Exception:
            np = None
    if load_faiss and faiss is None:
        try:
            import faiss as faiss_module
            faiss = faiss_module
        except Exception:
            faiss = None
    if load_model and SentenceTransformer is None:
        try:
            from sentence_transformers import SentenceTransformer as model_class
            SentenceTransformer = model_class
        except Exception:
            SentenceTransformer = None
    return np, faiss, SentenceTransformer


ROOT_DIR = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE = ROOT_DIR / "main" / "campus_data.cc"
DEFAULT_JSON = ROOT_DIR / "data" / "campus_data.json"

ENTRY_PATTERN = re.compile(
    r'\{\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*\}',
    re.DOTALL,
)

SUMMARY_DOSEN_IDS = {"dosen_fti", "dosen_feb", "dosen_fps", "dosen_mku"}

STOPWORDS = {
    "apa", "apakah", "siapa", "siapakah", "yang", "dan", "atau", "di", "ke", "dari",
    "dengan", "untuk", "tentang", "dong", "nih", "nya", "ya", "kah", "itu", "ini",
    "ucic", "cic", "kampus", "universitas", "catur", "insan", "cendekia", "tolong",
    "mohon", "saya", "aku", "kamu", "mau", "ingin", "info", "informasi", "jelaskan",
    "dosen", "pengampu", "ngajar", "mengajar", "diajar", "mata", "kuliah", "matkul",
}

PRODI_ALIASES = {
    "teknik informatika": ("teknik informatika", " ti "),
    "sistem informasi": ("sistem informasi", " si "),
    "desain komunikasi visual": ("desain komunikasi visual", " dkv "),
    "akuntansi": ("akuntansi", " akt "),
    "bisnis digital": ("bisnis digital", " bisdi ", " bisdig "),
    "pendidikan kepelatihan keolahragaan": ("pendidikan kepelatihan keolahragaan", " pikor ", " pkor "),
    "manajemen informatika": ("manajemen informatika", " d3 mi ", " mi "),
    "manajemen bisnis": ("manajemen bisnis", " d3 mb ", " mb "),
    "manajemen": ("manajemen", " mjn "),
}

PUBLIC_REALTIME_TERMS = {
    "presiden", "wakil presiden", "gubernur", "wakil gubernur", "menteri", "bupati",
    "wali kota", "walikota", "pejabat", "kepala daerah", "kapolri", "panglima",
    "jokowi", "prabowo", "ridwan kamil", "dedi mulyadi",
}

CAMPUS_DEFAULT_TERMS = {
    "ucic", "cic", "catur insan", "cendekia", "rektor", "dosen", "prodi", "program studi",
    "jurusan", "fakultas", "akreditasi", "biaya", "pmb", "beasiswa", "kurikulum",
    "semester", "mata kuliah", "matkul", "kampus", "alamat", "kontak", "whatsapp",
    "fasilitas", "visi", "misi", "great", "karakter", "nilai", "dpp", "gedung",
    "ngajar", "mengajar", "diajar", "pengampu", "mengampu", "kenal", "agama",
}

SHORT_ALLOW = {
    "ti", "si", "mi", "mb", "dkv", "akt", "mjn", "ai", "ui", "ux", "wa", "erp",
    "seo", "imc", "rpl", "dpp", "fti", "feb", "fps",
}

TYPO_REPLACEMENTS = {
    "pemograman": "pemrograman",
    "pemrogramman": "pemrograman",
    "programan": "pemrograman",
    "algoritma pemograman": "algoritma pemrograman",
    "algoritma dan pemograman": "algoritma dan pemrograman",
    "krecitan": "kristen",
    "keristen": "kristen",
    "kristian": "kristen",
    "katolik": "katolik",
    "budha": "budha",
    "buddha": "budha",
    "kong hu cu": "konghucu",
    "kong hu chu": "konghucu",
    "kong hu": "konghucu",
    "rektorat": "rektor",
    "rector": "rektor",
    "dosenya": "dosennya",
    "dosem": "dosen",
    "dosan": "dosen",
    "matakuliah": "mata kuliah",
    "prodi": "program studi",
    "jurusannya": "jurusan",
    "akreditas": "akreditasi",
    "wa": "whatsapp",
    "pkor": "pikor",
}


def decode_c_string(value: str) -> str:
    return ast.literal_eval('"' + value + '"')


def parse_campus_source(source_path: Path = DEFAULT_SOURCE) -> list[dict[str, str]]:
    source = source_path.read_text(encoding="utf-8")
    marker = "static const CampusEntry CAMPUS_DB[] = {"
    start = source.find(marker)
    if start < 0:
        raise RuntimeError("Marker CAMPUS_DB tidak ditemukan")
    end = source.find("};", start)
    if end < 0:
        raise RuntimeError("Akhir CAMPUS_DB tidak ditemukan")

    entries: list[dict[str, str]] = []
    for match in ENTRY_PATTERN.finditer(source[start:end]):
        entries.append(
            {
                "id": decode_c_string(match.group(1)),
                "category": decode_c_string(match.group(2)),
                "title": decode_c_string(match.group(3)),
                "content": decode_c_string(match.group(4)),
                "keywords": decode_c_string(match.group(5)),
            }
        )
    if not entries:
        raise RuntimeError("Tidak ada entri kampus yang berhasil dibaca")
    return entries


def load_entries(data_path: Path = DEFAULT_JSON, source_path: Path = DEFAULT_SOURCE) -> list[dict[str, str]]:
    if data_path.exists():
        return json.loads(data_path.read_text(encoding="utf-8"))
    return parse_campus_source(source_path)


def normalize_text(text: str) -> str:
    value = text.lower()
    value = value.replace("&", " dan ")
    for wrong, right in TYPO_REPLACEMENTS.items():
        value = value.replace(wrong, right)
    value = re.sub(r"[^a-z0-9]+", " ", value)
    return re.sub(r"\s+", " ", value).strip()


def tokenize(text: str) -> list[str]:
    result: list[str] = []
    for token in normalize_text(text).split():
        if token in STOPWORDS:
            continue
        if len(token) <= 1:
            continue
        if len(token) == 2 and token not in SHORT_ALLOW and not token.isdigit():
            continue
        result.append(token)
    return result


def contains_token(text: str, token: str) -> bool:
    return token in set(tokenize(text))


def mentioned_prodi(query_norm: str) -> str:
    padded = f" {query_norm} "
    for canonical, aliases in PRODI_ALIASES.items():
        if any(alias.strip() in query_norm if len(alias.strip()) > 2 else alias in padded for alias in aliases):
            return canonical
    return ""


def looks_like_campus_domain(query: str) -> bool:
    q = normalize_text(query)
    if any(term in q for term in CAMPUS_DEFAULT_TERMS):
        if any(term in q for term in PUBLIC_REALTIME_TERMS):
            return any(term in q for term in ("ucic", "cic", "kampus", "rektor", "dosen", "fakultas", "jurusan", "prodi"))
        return True
    return False


def fuzzy_ratio(a: str, b: str) -> float:
    if not a or not b:
        return 0.0
    if rapid_fuzz is not None:
        return float(rapid_fuzz.token_set_ratio(a, b)) / 100.0
    return SequenceMatcher(None, a, b).ratio()


def split_courses(content: str) -> list[str]:
    if ":" not in content:
        return []
    block = content.split(":", 1)[1].replace("\n", ",")
    courses: list[str] = []
    buf: list[str] = []
    depth = 0
    for char in block:
        if char == "(":
            depth += 1
        elif char == ")" and depth > 0:
            depth -= 1
        if char == "," and depth == 0:
            item = "".join(buf).strip(" .")
            if item:
                courses.append(item)
            buf = []
        else:
            buf.append(char)
    tail = "".join(buf).strip(" .")
    if tail:
        courses.append(tail)
    return courses


def lecturer_name(title: str) -> str:
    return title.strip()


@dataclass
class Chunk:
    chunk_id: str
    entry_id: str
    category: str
    title: str
    text: str
    answer: str
    keywords: str
    chunk_type: str = "entry"
    lecturer: str = ""
    course: str = ""


def build_chunks(entries: list[dict[str, str]]) -> list[Chunk]:
    chunks: list[Chunk] = []
    for entry in entries:
        entry_text = " ".join(
            [entry["id"], entry["category"], entry["title"], entry["content"], entry["keywords"]]
        )
        chunks.append(
            Chunk(
                chunk_id=entry["id"],
                entry_id=entry["id"],
                category=entry["category"],
                title=entry["title"],
                text=entry_text,
                answer=entry["content"],
                keywords=entry["keywords"],
            )
        )

        if entry["category"] == "dosen" and entry["id"] not in SUMMARY_DOSEN_IDS:
            name = lecturer_name(entry["title"])
            for index, course in enumerate(split_courses(entry["content"])):
                chunks.append(
                    Chunk(
                        chunk_id=f"{entry['id']}::course::{index}",
                        entry_id=entry["id"],
                        category="dosen",
                        title=entry["title"],
                        text=f"dosen pengampu mata kuliah {course} {name} {entry['content']} {entry['keywords']}",
                        answer=f"Mata kuliah {course} diajar oleh {name}.",
                        keywords=f"{entry['keywords']},{course}",
                        chunk_type="lecturer_course",
                        lecturer=name,
                        course=course,
                    )
                )
    return chunks


def hash_vector(text: str, dims: int = 384) -> list[float]:
    normalized = normalize_text(text)
    vector = [0.0] * dims
    if not normalized:
        return vector
    padded = f"  {normalized}  "
    for width in (3, 4, 5):
        for index in range(0, max(0, len(padded) - width + 1)):
            gram = padded[index:index + width]
            bucket = hash(gram) % dims
            vector[bucket] += 1.0
    norm = math.sqrt(sum(value * value for value in vector)) or 1.0
    return [value / norm for value in vector]


def cosine(left: list[float], right: list[float]) -> float:
    return sum(a * b for a, b in zip(left, right))


def text_score(query: str, chunk: Chunk) -> float:
    q_norm = normalize_text(query)
    c_norm = normalize_text(chunk.text)
    q_tokens = tokenize(query)
    if not q_tokens:
        return 0.0

    score = 0.0
    if q_norm and q_norm in c_norm:
        score += 1.3
    title_norm = normalize_text(chunk.title)
    keyword_norm = normalize_text(chunk.keywords)
    course_norm = normalize_text(chunk.course)
    answer_norm = normalize_text(chunk.answer)
    for token in q_tokens:
        if token in title_norm:
            score += 0.32
        if token in keyword_norm:
            score += 0.28
        if token in course_norm:
            score += 0.42
        if token in answer_norm:
            score += 0.18
        if token in c_norm:
            score += 0.08
    return min(score, 4.0) / 4.0


def intent_boost(query: str, chunk: Chunk) -> float:
    q = normalize_text(query)
    boost = 0.0

    if "rektor" in q:
        boost += 1.8 if chunk.entry_id == "rektor" else -0.8

    asks_lecturer = any(term in q for term in ("dosen", "ngajar", "mengajar", "diajar", "pengampu", "kenal"))
    if asks_lecturer:
        boost += 0.65 if chunk.category == "dosen" else -0.3
        if chunk.chunk_type == "lecturer_course":
            boost += 0.45

    if "agama" in q:
        course_norm = normalize_text(chunk.course + " " + chunk.answer)
        if "islam" in q:
            boost += 1.6 if "agama islam" in course_norm else -0.4
        elif any(term in q for term in ("kristen", "katolik", "budha", "hindu", "konghucu")):
            wanted = [term for term in ("kristen", "katolik", "budha", "hindu", "konghucu") if term in q]
            boost += 1.8 if any(term in course_norm for term in wanted) else -0.5
        else:
            boost += 1.2 if "pendidikan agama" in course_norm else -0.25

    if "algoritma" in q and "pemrograman" in q:
        course_norm = normalize_text(chunk.course)
        asks_two = " 2" in f" {q} " or " dua" in f" {q} "
        if asks_two:
            boost += 1.6 if "algoritma dan pemrograman 2" in course_norm else -0.7
        else:
            basic = "algoritma pemrograman" in course_norm or "algoritma dan pemrograman" in course_norm
            has_two = "pemrograman 2" in course_norm
            boost += 1.2 if basic and not has_two else -0.45 if has_two else 0.0

    if "akreditasi" in q:
        boost += 1.55 if chunk.category == "akreditasi" else -0.55
        if any(term in q for term in ("teknik informatika", "sistem informasi", "dkv", "akuntansi", "manajemen", "bisnis digital", "pikor", "manajemen informatika", "manajemen bisnis")):
            boost += 0.8 if chunk.entry_id == "akreditasi_prodi" else -0.15

    if any(term in q for term in ("biaya", "pmb", "dpp", "gedung", "total", "opsi")):
        boost += 1.45 if chunk.category == "biaya" else -0.55
        if "opsi 1" in q or "3 bulan" in q:
            boost += 1.0 if chunk.entry_id == "biaya_pmb_opsi1" else -0.2
        if "opsi 2" in q or "6 bulan" in q or "1 semester" in q:
            boost += 1.0 if chunk.entry_id == "biaya_pmb_opsi2" else -0.2
        if "opsi 3" in q or "1 tahun" in q or "2 semester" in q:
            boost += 1.0 if chunk.entry_id == "biaya_pmb_opsi3" else -0.2

    asks_lecturer_detail = any(
        phrase in q
        for phrase in (
            "ngajar apa", "mengajar apa", "ampu apa", "mengampu apa",
            "mata kuliah apa", "matkul apa", "dosen ini",
        )
    )
    if asks_lecturer_detail:
        boost += 1.1 if chunk.category == "dosen" and chunk.chunk_type == "entry" else -0.25

    return boost


class CampusRag:
    def __init__(self, entries: list[dict[str, str]], embedding_model: str = "", use_faiss: bool = False):
        self.entries = entries
        self.chunks = build_chunks(entries)
        self.entry_chunks = {
            chunk.entry_id: chunk
            for chunk in self.chunks
            if chunk.chunk_type == "entry"
        }
        self.entry_norms = [
            {
                "id": entry["id"],
                "title": normalize_text(entry["title"]),
                "keywords": normalize_text(entry["keywords"]),
                "tokens": set(tokenize(entry["title"] + " " + entry["keywords"])),
            }
            for entry in entries
        ]
        self.embedding_model_name = embedding_model
        self.model = None
        self.faiss_index = None
        self.vectors: list[list[float]] = []
        self._build_vector_index(use_faiss)

    def _build_vector_index(self, use_faiss: bool) -> None:
        texts = [chunk.text for chunk in self.chunks]
        numpy_module, faiss_module, model_class = lazy_import_vector_dependencies(
            bool(self.embedding_model_name), use_faiss
        )
        if self.embedding_model_name and SentenceTransformer is not None and np is not None:
            self.model = model_class(self.embedding_model_name)
            encoded = self.model.encode(texts, normalize_embeddings=True, show_progress_bar=False)
            matrix = numpy_module.asarray(encoded, dtype="float32")
            self.vectors = matrix.tolist()
            if use_faiss and faiss_module is not None:
                self.faiss_index = faiss_module.IndexFlatIP(matrix.shape[1])
                self.faiss_index.add(matrix)
            return

        self.vectors = [hash_vector(text) for text in texts]
        if use_faiss and faiss_module is not None and numpy_module is not None:
            matrix = numpy_module.asarray(self.vectors, dtype="float32")
            self.faiss_index = faiss_module.IndexFlatIP(matrix.shape[1])
            self.faiss_index.add(matrix)

    def _query_vector(self, query: str) -> list[float]:
        if self.model is not None:
            encoded = self.model.encode([query], normalize_embeddings=True, show_progress_bar=False)
            return encoded[0].tolist()
        return hash_vector(query)

    def _vector_scores(self, query: str, top_k: int = 24) -> dict[int, float]:
        q_vec = self._query_vector(query)
        if self.faiss_index is not None and np is not None:
            matrix = np.asarray([q_vec], dtype="float32")
            distances, indexes = self.faiss_index.search(matrix, min(top_k, len(self.chunks)))
            return {int(idx): float(score) for idx, score in zip(indexes[0], distances[0]) if idx >= 0}
        return {idx: cosine(q_vec, vector) for idx, vector in enumerate(self.vectors)}

    def retrieve(self, query: str, top_k: int = 6) -> list[tuple[Chunk, float]]:
        q_norm = normalize_text(query)
        vector_scores = self._vector_scores(q_norm)
        scored: list[tuple[Chunk, float]] = []
        for idx, chunk in enumerate(self.chunks):
            lexical = text_score(q_norm, chunk)
            vector = vector_scores.get(idx, 0.0)
            fuzzy = fuzzy_ratio(q_norm, normalize_text(chunk.title + " " + chunk.course + " " + chunk.keywords))
            boost = intent_boost(q_norm, chunk)
            score = lexical * 2.4 + vector * 1.4 + fuzzy * 0.9 + boost
            if score > 0.16:
                scored.append((chunk, score))
        scored.sort(key=lambda item: item[1], reverse=True)
        return self._dedupe(scored, top_k)

    @staticmethod
    def _dedupe(scored: list[tuple[Chunk, float]], top_k: int) -> list[tuple[Chunk, float]]:
        result: list[tuple[Chunk, float]] = []
        seen: set[str] = set()
        for chunk, score in scored:
            key = chunk.chunk_id if chunk.chunk_type == "lecturer_course" else chunk.entry_id
            if key in seen:
                continue
            seen.add(key)
            result.append((chunk, score))
            if len(result) >= top_k:
                break
        return result

    def answer(self, query: str) -> dict[str, Any]:
        q_norm = normalize_text(query)
        exact_entry_id = self._exact_entry_id(q_norm)
        if exact_entry_id:
            selected = self._single_entry_selection(exact_entry_id, [])
            if selected:
                confidence = 0.99
                answer = self._shorten_text(self._compose_answer(q_norm, selected), 5200)
                payload_matches = [
                    {
                        "id": selected[0][0].entry_id,
                        "chunk_id": selected[0][0].chunk_id,
                        "title": selected[0][0].title,
                        "category": selected[0][0].category,
                        "course": selected[0][0].course,
                        "score": round(selected[0][1], 4),
                    }
                ]
                tool_text = self._tool_text(answer, confidence, payload_matches)
                return {
                    "status": "found",
                    "source": "campus_rag_server",
                    "confidence": confidence,
                    "answer": answer,
                    "matches": payload_matches,
                    "tool_text": tool_text,
                }

        if not looks_like_campus_domain(query) and not self._looks_like_known_entry_query(q_norm):
            return {
                "status": "not_found",
                "source": "campus_rag_server",
                "confidence": 0.0,
                "answer": "Pertanyaan ini tidak terdeteksi sebagai pertanyaan data kampus UCIC.",
                "matches": [],
            }

        forced_entry_id = self._forced_entry_id(q_norm)
        matches = self.retrieve(query, top_k=8)
        if forced_entry_id:
            selected = self._single_entry_selection(forced_entry_id, matches)
            if selected:
                confidence = 0.99
                answer = self._shorten_text(self._compose_answer(q_norm, selected), 5200)
                payload_matches = [
                    {
                        "id": selected[0][0].entry_id,
                        "chunk_id": selected[0][0].chunk_id,
                        "title": selected[0][0].title,
                        "category": selected[0][0].category,
                        "course": selected[0][0].course,
                        "score": round(selected[0][1], 4),
                    }
                ]
                tool_text = self._tool_text(answer, confidence, payload_matches)
                return {
                    "status": "found",
                    "source": "campus_rag_server",
                    "confidence": confidence,
                    "answer": answer,
                    "matches": payload_matches,
                    "tool_text": tool_text,
                }

        if not matches or matches[0][1] < 0.55:
            return {
                "status": "not_found",
                "source": "campus_rag_server",
                "confidence": 0.0,
                "answer": f"Data kampus UCIC yang cocok belum ditemukan untuk pertanyaan: {query}",
                "matches": [],
            }

        confidence = min(0.99, max(0.35, matches[0][1] / 4.2))
        selected = self._select_answer_chunks(q_norm, matches)
        answer = self._shorten_text(self._compose_answer(q_norm, selected), 5200)
        payload_matches = [
            {
                "id": chunk.entry_id,
                "chunk_id": chunk.chunk_id,
                "title": chunk.title,
                "category": chunk.category,
                "course": chunk.course,
                "score": round(score, 4),
            }
            for chunk, score in selected[:5]
        ]
        tool_text = self._tool_text(answer, confidence, payload_matches)
        return {
            "status": "found",
            "source": "campus_rag_server",
            "confidence": round(confidence, 4),
            "answer": answer,
            "matches": payload_matches,
            "tool_text": tool_text,
        }

    def _exact_entry_id(self, query_norm: str) -> str:
        if not query_norm:
            return ""
        query_words = query_norm.split()
        for item in self.entry_norms:
            title_norm = item["title"]
            if query_norm == title_norm:
                return item["id"]
            if len(query_words) >= 2 and query_norm in title_norm:
                return item["id"]
        return ""

    def _looks_like_known_entry_query(self, query_norm: str) -> bool:
        query_tokens = set(tokenize(query_norm))
        if not query_tokens:
            return False
        for item in self.entry_norms:
            hits = len(query_tokens & item["tokens"])
            if hits >= 2:
                return True
            if hits == 1 and len(query_tokens) == 1:
                token = next(iter(query_tokens))
                if len(token) >= 5 and (token in item["title"] or token in item["keywords"]):
                    return True
        return False

    @staticmethod
    def _forced_entry_id(query_norm: str) -> str:
        if any(term in query_norm for term in ("dosen", "ngajar", "mengajar", "diajar", "pengampu", "kenal")):
            return ""

        if "rektor" in query_norm:
            return "rektor"
        if any(term in query_norm for term in ("jurusan yang cocok", "rekomendasi jurusan", "prodi yang cocok", "cocok buat saya", "cocok untuk saya")):
            return "faq_ucic"
        if "visi" in query_norm or "misi" in query_norm:
            return "visi_misi"
        if any(term in query_norm for term in ("logo", "lambang", "filosofi")):
            return "logo_ucic"
        if any(term in query_norm for term in ("alamat", "lokasi", "dimana")):
            return "lokasi_kampus"
        if any(term in query_norm for term in ("kontak", "whatsapp", "telepon", "email")):
            return "kontak_ucic"
        if any(term in query_norm for term in ("fasilitas", "lab", "wifi")):
            return "fasilitas"
        if "beasiswa" in query_norm:
            return "beasiswa"
        if any(term in query_norm for term in ("magang", "internship")):
            return "magang"
        if any(term in query_norm for term in ("singkatan", "kepanjangan")):
            return "singkatan_prodi"
        if "akreditasi" in query_norm:
            if mentioned_prodi(query_norm) or any(term in query_norm for term in ("program studi", "prodi", "jurusan")):
                return "akreditasi_prodi"
            return "akreditasi"
        if any(term in query_norm for term in ("pendaftaran", "pmb", "mahasiswa baru", "cara daftar", "registrasi", "syarat daftar", "cara masuk")):
            if not any(term in query_norm for term in ("biaya", "dpp", "gedung", "total", "opsi", "tahap")):
                return "pendaftaran_pmb"
        if any(term in query_norm for term in ("profil", "sejarah", "tentang")):
            return "profil_ucic"
        if any(term in query_norm for term in ("cic great", "great", "nilai dasar", "nilai kampus", "karakter kampus", "budaya kampus", "core value")):
            return "great_values"
        if any(term in query_norm for term in ("fti", "teknologi informasi")):
            return "jurusan_fti"
        if any(term in query_norm for term in ("feb", "ekonomi dan bisnis")):
            return "jurusan_feb"
        if any(term in query_norm for term in ("fps", "pendidikan dan sains")):
            return "jurusan_fps"
        if any(term in query_norm for term in ("jurusan", "fakultas", "program studi", "prodi")):
            if mentioned_prodi(query_norm):
                return ""
            if not any(term in query_norm for term in ("beda", "bedanya", "perbedaan", "bandingkan", " vs ")):
                return "jurusan_ucic"
        return ""

    def _single_entry_selection(
        self, entry_id: str, matches: list[tuple[Chunk, float]]
    ) -> list[tuple[Chunk, float]]:
        for chunk, score in matches:
            if chunk.entry_id == entry_id and chunk.chunk_type == "entry":
                return [(chunk, max(score, 4.0))]
        chunk = self.entry_chunks.get(entry_id)
        if chunk is None:
            return []
        return [(chunk, 4.0)]

    def _select_answer_chunks(
        self, query_norm: str, matches: list[tuple[Chunk, float]]
    ) -> list[tuple[Chunk, float]]:
        best_score = matches[0][1]

        if "rektor" in query_norm:
            return [item for item in matches if item[0].entry_id == "rektor"][:1] or matches[:1]

        if "agama" in query_norm:
            filtered: list[tuple[Chunk, float]] = []
            for chunk, score in matches:
                if chunk.chunk_type != "lecturer_course":
                    continue
                course_norm = normalize_text(chunk.course)
                if "islam" in query_norm and "agama islam" in course_norm:
                    filtered.append((chunk, score))
                elif any(term in query_norm for term in ("kristen", "katolik", "budha", "hindu", "konghucu")):
                    if any(term in course_norm for term in ("kristen", "katolik", "budha", "hindu", "konghucu")):
                        filtered.append((chunk, score))
                elif "pendidikan agama" in course_norm:
                    filtered.append((chunk, score))
            if filtered:
                return self._unique_lecturers(filtered)

        if "algoritma" in query_norm and "pemrograman" in query_norm:
            asks_two = " 2" in f" {query_norm} " or " dua" in f" {query_norm} "
            filtered = []
            for chunk, score in matches:
                if chunk.chunk_type != "lecturer_course":
                    continue
                course_norm = normalize_text(chunk.course)
                if asks_two and "algoritma dan pemrograman 2" in course_norm:
                    filtered.append((chunk, score))
                if not asks_two and (
                    "algoritma pemrograman" in course_norm or "algoritma dan pemrograman" in course_norm
                ) and "pemrograman 2" not in course_norm:
                    filtered.append((chunk, score))
            if filtered:
                return self._unique_lecturers(filtered)

        asks_lecturer_detail = any(
            phrase in query_norm
            for phrase in (
                "ngajar apa", "mengajar apa", "ampu apa", "mengampu apa",
                "mata kuliah apa", "matkul apa", "dosen ini",
            )
        )
        if asks_lecturer_detail:
            lecturer_entries = [
                item for item in matches
                if item[0].category == "dosen" and item[0].chunk_type == "entry"
            ]
            if lecturer_entries:
                return lecturer_entries[:1]

        if "akreditasi" in query_norm:
            akreditasi = [item for item in matches if item[0].category == "akreditasi"]
            if akreditasi:
                if mentioned_prodi(query_norm):
                    prodi_entries = [item for item in akreditasi if item[0].entry_id == "akreditasi_prodi"]
                    return prodi_entries[:1] or akreditasi[:1]
                return akreditasi[:2]

        if any(term in query_norm for term in ("biaya", "pmb", "dpp", "gedung", "total", "opsi")):
            biaya = [item for item in matches if item[0].category == "biaya"]
            if biaya:
                if "opsi 1" in query_norm or "3 bulan" in query_norm:
                    return [item for item in biaya if item[0].entry_id == "biaya_pmb_opsi1"][:1] or biaya[:1]
                if "opsi 2" in query_norm or "6 bulan" in query_norm or "1 semester" in query_norm:
                    return [item for item in biaya if item[0].entry_id == "biaya_pmb_opsi2"][:1] or biaya[:1]
                if "opsi 3" in query_norm or "1 tahun" in query_norm or "2 semester" in query_norm:
                    return [item for item in biaya if item[0].entry_id == "biaya_pmb_opsi3"][:1] or biaya[:1]
                return biaya[:2]

        lecturer_matches = [item for item in matches if item[0].chunk_type == "lecturer_course"]
        if lecturer_matches and any(term in query_norm for term in ("dosen", "ngajar", "mengajar", "diajar", "pengampu")):
            return self._unique_lecturers(
                [item for item in lecturer_matches if item[1] >= max(0.75, best_score * 0.55)]
            )

        return [item for item in matches if item[1] >= max(0.7, best_score * 0.58)][:4]

    @staticmethod
    def _unique_lecturers(matches: list[tuple[Chunk, float]]) -> list[tuple[Chunk, float]]:
        selected: list[tuple[Chunk, float]] = []
        seen: set[str] = set()
        for chunk, score in matches:
            if chunk.entry_id in seen:
                continue
            seen.add(chunk.entry_id)
            selected.append((chunk, score))
        return selected

    @staticmethod
    def _compose_answer(query_norm: str, selected: list[tuple[Chunk, float]]) -> str:
        if not selected:
            return "Data kampus UCIC yang cocok belum ditemukan."
        chunks = [item[0] for item in selected]

        if all(chunk.category == "dosen" for chunk in chunks) and any(chunk.chunk_type == "lecturer_course" for chunk in chunks):
            names = [chunk.lecturer or chunk.title for chunk in chunks]
            if len(names) == 1:
                return f"Dosen pengampu yang relevan adalah {names[0]}."
            return "Dosen pengampu yang relevan adalah " + ", ".join(names[:-1]) + f", dan {names[-1]}."

        if len(chunks) == 1:
            answer = CampusRag._filter_entry_answer(query_norm, chunks[0])
            return f"{chunks[0].title}: {answer}"

        parts = [f"{chunk.title}: {chunk.answer}" for chunk in chunks]
        return "\n".join(parts)

    @staticmethod
    def _filter_entry_answer(query_norm: str, chunk: Chunk) -> str:
        prodi = mentioned_prodi(query_norm)
        if chunk.category == "akreditasi" and prodi:
            for line in chunk.answer.splitlines():
                if prodi in normalize_text(line):
                    return line.strip(" -")
        if chunk.category == "biaya" and prodi:
            lines = chunk.answer.splitlines()
            intro = lines[0] if lines else ""
            matches = []
            wants_sore = "sore" in query_norm
            wants_rpi = "rpi" in query_norm
            for line in lines:
                line_norm = normalize_text(line)
                if prodi not in line_norm:
                    continue
                if "sore" in line_norm and not wants_sore:
                    continue
                if "rpi" in line_norm and not wants_rpi:
                    continue
                matches.append(line.strip())
            if matches:
                return "\n".join([intro] + matches)
        return chunk.answer

    @staticmethod
    def _shorten_text(text: str, max_chars: int) -> str:
        if len(text) <= max_chars:
            return text

        cut = text.rfind("\n", 0, max_chars)
        if cut < max_chars // 2:
            cut = text.rfind(" ", 0, max_chars)
        if cut < max_chars // 2:
            cut = max_chars

        return (
            text[:cut].rstrip()
            + "\n\nCatatan: jawaban dipersingkat agar perangkat tetap stabil. "
            "Untuk rincian lengkap, tanyakan dengan kata kunci yang lebih spesifik."
        )

    @staticmethod
    def _tool_text(answer: str, confidence: float, matches: list[dict[str, Any]]) -> str:
        evidence = ", ".join(item["title"] for item in matches[:3])
        return (
            "Data kampus UCIC dari Campus RAG Server. Jawab dalam bahasa Indonesia penuh, "
            "faktual, dan hanya berdasarkan Jawaban Wajib serta Evidence berikut. "
            "Jangan menambah data di luar hasil retrieval ini.\n\n"
            f"Jawaban Wajib:\n{answer}\n\n"
            f"Confidence: {confidence:.2f}\n"
            f"Evidence: {evidence}\n"
        )


class CampusRagHandler(BaseHTTPRequestHandler):
    rag: CampusRag

    def log_message(self, fmt: str, *args: Any) -> None:
        sys.stderr.write("[%s] %s\n" % (self.log_date_time_string(), fmt % args))

    def _send_json(self, payload: dict[str, Any], status: int = 200) -> None:
        raw = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(raw)))
        self.end_headers()
        self.wfile.write(raw)

    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path == "/health":
            self._send_json({"status": "ok", "entries": len(self.rag.entries), "chunks": len(self.rag.chunks)})
            return
        if parsed.path != "/query":
            self._send_json({"status": "not_found", "message": "Gunakan /query?q=pertanyaan"}, 404)
            return
        query = unquote_plus(parse_qs(parsed.query).get("q", [""])[0]).strip()
        if not query:
            self._send_json({"status": "bad_request", "message": "Parameter q wajib diisi"}, 400)
            return
        self._send_json(self.rag.answer(query))

    def do_POST(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path != "/query":
            self._send_json({"status": "not_found", "message": "Gunakan /query"}, 404)
            return
        length = int(self.headers.get("Content-Length", "0") or "0")
        body = self.rfile.read(length).decode("utf-8", errors="replace")
        try:
            payload = json.loads(body)
        except json.JSONDecodeError:
            self._send_json({"status": "bad_request", "message": "JSON tidak valid"}, 400)
            return
        query = str(payload.get("query", "")).strip()
        if not query:
            self._send_json({"status": "bad_request", "message": "Field query wajib diisi"}, 400)
            return
        self._send_json(self.rag.answer(query))


def local_ip_hint() -> str:
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.connect(("8.8.8.8", 80))
        value = sock.getsockname()[0]
        sock.close()
        return value
    except Exception:
        return "127.0.0.1"


def main() -> int:
    parser = argparse.ArgumentParser(description="Campus RAG Server UCIC")
    parser.add_argument("--data", default=str(DEFAULT_JSON), help="Path JSON data kampus")
    parser.add_argument("--source", default=str(DEFAULT_SOURCE), help="Fallback source campus_data.cc")
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8787)
    parser.add_argument("--embedding-model", default="", help="Nama model sentence-transformers opsional")
    parser.add_argument("--use-faiss", action="store_true", help="Aktifkan FAISS jika paketnya tersedia")
    parser.add_argument("--query", default="", help="Mode CLI: langsung tanya satu pertanyaan")
    parser.add_argument("--pretty", action="store_true", help="Cetak JSON rapi pada mode CLI")
    args = parser.parse_args()

    started = time.time()
    entries = load_entries(Path(args.data), Path(args.source))
    rag = CampusRag(entries, embedding_model=args.embedding_model, use_faiss=args.use_faiss)

    if args.query:
        result = rag.answer(args.query)
        print(json.dumps(result, ensure_ascii=False, indent=2 if args.pretty else None))
        return 0 if result["status"] == "found" else 1

    CampusRagHandler.rag = rag
    server = ThreadingHTTPServer((args.host, args.port), CampusRagHandler)
    print(f"Campus RAG Server siap: {len(entries)} entri, {len(rag.chunks)} chunk, {time.time() - started:.2f} detik")
    print(f"URL untuk ESP32 di jaringan lokal: http://{local_ip_hint()}:{args.port}")
    print("Endpoint: GET /query?q=pertanyaan atau POST /query {\"query\":\"...\"}")
    server.serve_forever()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
