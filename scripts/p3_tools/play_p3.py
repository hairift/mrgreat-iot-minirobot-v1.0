import argparse
import struct

import numpy as np
import opuslib
import sounddevice as sd


def play_p3_file(input_file):
    """
    Memutar berkas audio berformat p3.

    Format p3:
    [1 byte tipe, 1 byte cadangan, 2 byte panjang, data Opus]
    """
    # Inisialisasi dekoder Opus.
    sample_rate = 16000  # Laju sampling tetap 16000 Hz.
    channels = 1  # Mono.
    decoder = opuslib.Decoder(sample_rate, channels)

    # Ukuran frame 60 ms.
    frame_size = int(sample_rate * 60 / 1000)

    # Buka aliran audio untuk keluaran.
    stream = sd.OutputStream(
        samplerate=sample_rate,
        channels=channels,
        dtype="int16",
    )
    stream.start()

    try:
        with open(input_file, "rb") as file_handle:
            print(f"Sedang memutar: {input_file}")

            while True:
                # Baca header paket sepanjang 4 byte.
                header = file_handle.read(4)
                if not header or len(header) < 4:
                    break

                # Uraikan header paket.
                packet_type, reserved, data_len = struct.unpack(">BBH", header)
                _ = packet_type, reserved

                # Baca data Opus sesuai panjang paket.
                opus_data = file_handle.read(data_len)
                if not opus_data or len(opus_data) < data_len:
                    break

                # Dekode data Opus menjadi PCM.
                pcm_data = decoder.decode(opus_data, frame_size)

                # Ubah byte PCM menjadi array NumPy.
                audio_array = np.frombuffer(pcm_data, dtype=np.int16)

                # Kirim audio ke perangkat keluaran.
                stream.write(audio_array)

    except KeyboardInterrupt:
        print("\nPemutaran dihentikan")
    finally:
        stream.stop()
        stream.close()
        print("Pemutaran selesai")


def main():
    parser = argparse.ArgumentParser(description="Memutar berkas audio berformat p3")
    parser.add_argument("input_file", help="Jalur berkas p3 masukan")
    args = parser.parse_args()

    play_p3_file(args.input_file)


if __name__ == "__main__":
    main()
