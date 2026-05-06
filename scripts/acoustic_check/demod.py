"""
Demodulator AFSK waktu nyata berbasis algoritma Goertzel.
"""

from collections import deque

import numpy as np


class TraceGoertzel:
    """Implementasi algoritma Goertzel untuk pemrosesan waktu nyata."""

    def __init__(self, freq: float, n: int):
        """
        Menyiapkan parameter Goertzel.

        Argumen:
            freq: Frekuensi ternormalisasi (frekuensi target / frekuensi sampling).
            n: Ukuran jendela.
        """
        self.freq = freq
        self.n = n

        # Hitung koefisien awal agar sesuai dengan rumus referensi.
        self.k = int(freq * n)
        self.w = 2.0 * np.pi * freq
        self.cw = np.cos(self.w)
        self.sw = np.sin(self.w)
        self.c = 2.0 * self.cw

        # Simpan dua keadaan terakhir dalam deque.
        self.zs = deque([0.0, 0.0], maxlen=2)

    def reset(self):
        """Mengatur ulang keadaan internal algoritma."""
        self.zs.clear()
        self.zs.extend([0.0, 0.0])

    def __call__(self, xs):
        """
        Memproses satu kumpulan sampel.

        Argumen:
            xs: Deret sampel.

        Hasil:
            Nilai amplitudo terhitung.
        """
        self.reset()
        for x in xs:
            z1, z2 = self.zs[-1], self.zs[-2]
            z0 = x + self.c * z1 - z2
            self.zs.append(float(z0))
        return self.amp

    @property
    def amp(self) -> float:
        """Menghitung amplitudo saat ini."""
        z1, z2 = self.zs[-1], self.zs[-2]
        ip = self.cw * z1 - z2
        qp = self.sw * z1
        return np.sqrt(ip**2 + qp**2) / (self.n / 2.0)


class PairGoertzel:
    """Demodulator Goertzel untuk dua frekuensi."""

    def __init__(self, f_sample: int, f_space: int, f_mark: int, bit_rate: int, win_size: int):
        """
        Menyiapkan demodulator dua frekuensi.

        Argumen:
            f_sample: Frekuensi sampling.
            f_space: Frekuensi space, biasanya mewakili bit 0.
            f_mark: Frekuensi mark, biasanya mewakili bit 1.
            bit_rate: Laju bit.
            win_size: Ukuran jendela Goertzel.
        """
        assert f_sample % bit_rate == 0, "Frekuensi sampling harus kelipatan laju bit"

        self.Fs = f_sample
        self.F0 = f_space
        self.F1 = f_mark
        self.bit_rate = bit_rate
        self.n_per_bit = int(f_sample // bit_rate)  # Jumlah sampel per bit.

        # Hitung frekuensi ternormalisasi.
        f1 = f_mark / f_sample
        f0 = f_space / f_sample

        # Inisialisasi dua pelacak Goertzel.
        self.g0 = TraceGoertzel(freq=f0, n=win_size)
        self.g1 = TraceGoertzel(freq=f1, n=win_size)

        # Penyangga masukan.
        self.in_buffer = deque(maxlen=win_size)
        self.out_count = 0

        print(
            f"PairGoertzel initialized: f0={f0:.6f}, f1={f1:.6f}, "
            f"win_size={win_size}, n_per_bit={self.n_per_bit}"
        )

    def __call__(self, sample: float):
        """
        Memproses satu sampel.

        Argumen:
            sample: Nilai sampel audio.

        Hasil:
            Tuple (amp0, amp1, p1_prob) berupa amplitudo space, amplitudo mark,
            dan probabilitas bit 1.
        """
        self.in_buffer.append(sample)
        self.out_count += 1

        amp0, amp1, p1_prob = 0, 0, None

        # Keluarkan hasil setiap satu periode bit.
        if self.out_count >= self.n_per_bit:
            amp0 = self.g0(self.in_buffer)
            amp1 = self.g1(self.in_buffer)
            p1_prob = amp1 / (amp0 + amp1 + 1e-8)
            self.out_count = 0

        return amp0, amp1, p1_prob


class RealTimeAFSKDecoder:
    """Dekoder AFSK waktu nyata berbasis pemicu bingkai awal."""

    def __init__(
        self,
        f_sample: int = 16000,
        mark_freq: int = 1800,
        space_freq: int = 1500,
        bitrate: int = 100,
        s_goertzel: int = 9,
        threshold: float = 0.5,
    ):
        """
        Menyiapkan dekoder AFSK waktu nyata.

        Argumen:
            f_sample: Frekuensi sampling.
            mark_freq: Frekuensi mark.
            space_freq: Frekuensi space.
            bitrate: Laju bit.
            s_goertzel: Faktor ukuran jendela Goertzel.
            threshold: Ambang keputusan bit.
        """
        self.f_sample = f_sample
        self.mark_freq = mark_freq
        self.space_freq = space_freq
        self.bitrate = bitrate
        self.threshold = threshold

        # Hitung ukuran jendela sesuai parameter mark.
        win_size = int(f_sample / mark_freq * s_goertzel)

        # Inisialisasi demodulator.
        self.demodulator = PairGoertzel(f_sample, space_freq, mark_freq, bitrate, win_size)

        # Definisi bingkai awal dan akhir.
        self.start_bytes = b"\x01\x02"
        self.end_bytes = b"\x03\x04"
        self.start_bits = "".join(format(int(x), "08b") for x in self.start_bytes)
        self.end_bits = "".join(format(int(x), "08b") for x in self.end_bytes)

        # Keadaan dekoder.
        self.state = "idle"  # idle / entering

        # Penyimpanan hasil demodulasi.
        self.buffer_prelude: deque = deque(maxlen=len(self.start_bits))
        self.indicators = []
        self.signal_bits = ""
        self.text_cache = ""

        # Statistik dekode.
        self.decoded_messages = []
        self.total_bits_received = 0

        print(f"Decoder initialized: win_size={win_size}")
        print(f"Start frame: {self.start_bits} (from {self.start_bytes.hex()})")
        print(f"End frame: {self.end_bits} (from {self.end_bytes.hex()})")

    def process_audio(self, samples: np.array) -> str:
        """
        Memproses sampel audio dan mengembalikan teks hasil dekode baru.

        Argumen:
            samples: Data audio PCM 16-bit yang sudah diubah menjadi array.

        Hasil:
            Teks baru yang berhasil didekode.
        """
        new_text = ""

        # Proses sampel satu per satu.
        for sample in samples:
            amp0, amp1, p1_prob = self.demodulator(sample)

            # Catat probabilitas bila ada keluaran dari demodulator.
            if p1_prob is not None:
                bit = "1" if p1_prob > self.threshold else "0"
                match self.state:
                    case "idle":
                        self.buffer_prelude.append(bit)
                    case "entering":
                        self.buffer_prelude.append(bit)
                        self.signal_bits += bit
                        self.total_bits_received += 1
                    case _:
                        pass
                self.indicators.append(p1_prob)

                # Periksa perpindahan keadaan.
                if self.state == "idle" and "".join(self.buffer_prelude) == self.start_bits:
                    self.state = "entering"
                    self.text_cache = ""
                    self.signal_bits = ""
                    self.buffer_prelude.clear()
                elif self.state == "entering" and (
                    "".join(self.buffer_prelude) == self.end_bits or len(self.signal_bits) >= 256
                ):
                    self.state = "idle"
                    self.buffer_prelude.clear()

                # Coba dekode setelah terkumpul minimal satu byte.
                if len(self.signal_bits) >= 8:
                    text = self._decode_bits_to_text(self.signal_bits)
                    if len(text) > len(self.text_cache):
                        new_text = text[len(self.text_cache) - len(text):]
                        self.text_cache = text

        return new_text

    def _decode_bits_to_text(self, bits: str) -> str:
        """
        Mengubah deret bit menjadi teks.

        Argumen:
            bits: Deret bit.

        Hasil:
            Teks hasil dekode.
        """
        if len(bits) < 8:
            return ""

        decoded_text = ""
        byte_count = len(bits) // 8

        for index in range(byte_count):
            # Ambil 8 bit untuk satu byte.
            byte_bits = bits[index * 8:(index + 1) * 8]

            # Ubah bit menjadi nilai byte.
            byte_val = int(byte_bits, 2)

            # Hanya tampilkan karakter ASCII yang bisa dicetak.
            if 32 <= byte_val <= 126:
                decoded_text += chr(byte_val)
            elif byte_val == 0:
                continue
            else:
                # Karakter non-cetak diabaikan.
                pass

        return decoded_text

    def clear(self):
        """Membersihkan keadaan dekoder."""
        self.indicators = []
        self.signal_bits = ""
        self.decoded_messages = []
        self.total_bits_received = 0
        print("Keadaan dekoder telah dibersihkan")

    def get_stats(self) -> dict:
        """Mengambil statistik dekoder saat ini."""
        return {
            "prelude_bits": "".join(self.buffer_prelude),
            "state": self.state,
            "total_chars": len(self.text_cache),
            "buffer_bits": len(self.signal_bits),
            "mark_freq": self.mark_freq,
            "space_freq": self.space_freq,
            "bitrate": self.bitrate,
            "threshold": self.threshold,
        }
