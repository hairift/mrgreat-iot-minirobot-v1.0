import argparse
import socket
import wave


"""
Buat socket UDP lalu kaitkan ke alamat server pada port 8000.
Dengarkan pesan masuk dan tampilkan ke konsol.
Simpan audio yang diterima ke berkas WAV.
"""


def main(samplerate, channels):
    # Buat socket UDP
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_socket.bind(("0.0.0.0", 8000))

    # Buat file WAV dengan parameter
    filename = f"{samplerate}_{channels}.wav"
    wav_file = wave.open(filename, "wb")
    wav_file.setnchannels(channels)  # parameter jumlah kanal
    wav_file.setsampwidth(2)  # 2 byte per sampel, yaitu 16-bit
    wav_file.setframerate(samplerate)  # parameter laju sampel

    print(f"Mulai menyimpan audio dari 0.0.0.0:8000 ke {filename}...")

    try:
        while True:
            # Terima pesan dari klien
            message, address = server_socket.recvfrom(8000)

            # Tulis data PCM ke file WAV
            wav_file.writeframes(message)

            # Cetak panjang pesan
            print(f"Diterima {len(message)} byte dari {address}")

    except KeyboardInterrupt:
        print("\nMenghentikan rekaman...")

    finally:
        # Tutup file dan socket
        wav_file.close()
        server_socket.close()
        print(f"File WAV '{filename}' berhasil disimpan")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Penerima data audio UDP yang menyimpan hasilnya sebagai berkas WAV"
    )
    parser.add_argument("--samplerate", "-s", type=int, default=16000, help="Laju sampel, bawaan 16000")
    parser.add_argument("--channels", "-c", type=int, default=2, help="Jumlah kanal, bawaan 2")

    args = parser.parse_args()
    main(args.samplerate, args.channels)
