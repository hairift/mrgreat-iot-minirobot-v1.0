import os
import struct
import threading
import time
import tkinter as tk
from tkinter import filedialog, messagebox

import numpy as np
import opuslib
import sounddevice as sd


def play_p3_file(input_file, stop_event=None, pause_event=None):
    """
    Putar berkas audio berformat P3.

    Format P3:
    [1 byte tipe, 1 byte cadangan, 2 byte panjang data, data Opus]
    """
    # Inisialisasi dekoder Opus.
    sample_rate = 16000  # Laju sampel tetap 16000 Hz.
    channels = 1  # Audio mono.
    decoder = opuslib.Decoder(sample_rate, channels)

    # Ukuran frame untuk durasi 60 ms.
    frame_size = int(sample_rate * 60 / 1000)

    # Buka aliran keluaran audio.
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
                if stop_event and stop_event.is_set():
                    break

                if pause_event and pause_event.is_set():
                    time.sleep(0.1)
                    continue

                # Baca header sepanjang 4 byte.
                header = file_handle.read(4)
                if not header or len(header) < 4:
                    break

                # Urai header paket.
                _, _, data_len = struct.unpack(">BBH", header)

                # Baca data Opus.
                opus_data = file_handle.read(data_len)
                if not opus_data or len(opus_data) < data_len:
                    break

                # Dekode data Opus ke PCM.
                pcm_data = decoder.decode(opus_data, frame_size)

                # Ubah byte PCM menjadi array NumPy.
                audio_array = np.frombuffer(pcm_data, dtype=np.int16)

                # Tulis audio ke perangkat keluaran.
                stream.write(audio_array)

    except KeyboardInterrupt:
        print("\nPemutaran dihentikan")
    finally:
        stream.stop()
        stream.close()
        print("Pemutaran selesai")


class P3PlayerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Pemutar Sederhana Berkas P3")
        self.root.geometry("500x400")

        self.playlist = []
        self.current_index = 0
        self.is_playing = False
        self.is_paused = False
        self.stop_event = threading.Event()
        self.pause_event = threading.Event()
        self.loop_playback = tk.BooleanVar(value=False)  # Menyimpan status putar berulang.

        # Bangun komponen antarmuka.
        self.create_widgets()

    def create_widgets(self):
        # Daftar putar.
        self.playlist_label = tk.Label(self.root, text="Daftar Putar:")
        self.playlist_label.pack(pady=5)

        self.playlist_frame = tk.Frame(self.root)
        self.playlist_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        self.playlist_listbox = tk.Listbox(self.playlist_frame, selectmode=tk.SINGLE)
        self.playlist_listbox.pack(fill=tk.BOTH, expand=True)

        # Area opsi dan tombol hapus.
        self.checkbox_frame = tk.Frame(self.root)
        self.checkbox_frame.pack(pady=5)

        self.remove_button = tk.Button(self.checkbox_frame, text="Hapus Berkas", command=self.remove_files)
        self.remove_button.pack(side=tk.LEFT, padx=5)

        # Opsi putar berulang.
        self.loop_checkbox = tk.Checkbutton(self.checkbox_frame, text="Putar Berulang", variable=self.loop_playback)
        self.loop_checkbox.pack(side=tk.LEFT, padx=5)

        # Tombol kontrol.
        self.control_frame = tk.Frame(self.root)
        self.control_frame.pack(pady=10)

        self.add_button = tk.Button(self.control_frame, text="Tambah Berkas", command=self.add_file)
        self.add_button.grid(row=0, column=0, padx=5)

        self.play_button = tk.Button(self.control_frame, text="Putar", command=self.play)
        self.play_button.grid(row=0, column=1, padx=5)

        self.pause_button = tk.Button(self.control_frame, text="Jeda", command=self.pause)
        self.pause_button.grid(row=0, column=2, padx=5)

        self.stop_button = tk.Button(self.control_frame, text="Hentikan", command=self.stop)
        self.stop_button.grid(row=0, column=3, padx=5)

        # Label status.
        self.status_label = tk.Label(self.root, text="Belum Memutar", fg="blue")
        self.status_label.pack(pady=10)

    def add_file(self):
        files = filedialog.askopenfilenames(filetypes=[("Berkas P3", "*.p3")])
        if files:
            self.playlist.extend(files)
            self.update_playlist()

    def update_playlist(self):
        self.playlist_listbox.delete(0, tk.END)
        for file_path in self.playlist:
            self.playlist_listbox.insert(tk.END, os.path.basename(file_path))  # Tampilkan nama berkas saja.

    def update_status(self, status_text, color="blue"):
        """Perbarui isi label status."""
        self.status_label.config(text=status_text, fg=color)

    def play(self):
        if not self.playlist:
            messagebox.showwarning("Peringatan", "Daftar putar masih kosong.")
            return

        if self.is_paused:
            self.is_paused = False
            self.pause_event.clear()
            self.update_status(f"Sedang memutar: {os.path.basename(self.playlist[self.current_index])}", "green")
            return

        if self.is_playing:
            return

        self.is_playing = True
        self.stop_event.clear()
        self.pause_event.clear()
        if self.playlist_listbox.curselection():
            self.current_index = self.playlist_listbox.curselection()[0]
        else:
            self.current_index = 0
        self.play_thread = threading.Thread(target=self.play_audio, daemon=True)
        self.play_thread.start()
        self.update_status(f"Sedang memutar: {os.path.basename(self.playlist[self.current_index])}", "green")

    def play_audio(self):
        while True:
            if self.stop_event.is_set():
                break

            if self.pause_event.is_set():
                time.sleep(0.1)
                continue

            # Pastikan indeks daftar putar masih valid.
            if self.current_index >= len(self.playlist):
                if self.loop_playback.get():  # Kembali ke awal bila putar berulang aktif.
                    self.current_index = 0
                else:
                    break

            file_path = self.playlist[self.current_index]
            self.playlist_listbox.selection_clear(0, tk.END)
            self.playlist_listbox.selection_set(self.current_index)
            self.playlist_listbox.activate(self.current_index)
            self.update_status(f"Sedang memutar: {os.path.basename(self.playlist[self.current_index])}", "green")
            play_p3_file(file_path, self.stop_event, self.pause_event)

            if self.stop_event.is_set():
                break

            if not self.loop_playback.get():  # Hentikan setelah berkas ini bila tidak berulang.
                break

            self.current_index += 1
            if self.current_index >= len(self.playlist) and self.loop_playback.get():
                self.current_index = 0

        self.is_playing = False
        self.is_paused = False
        self.update_status("Pemutaran dihentikan", "red")

    def pause(self):
        if self.is_playing:
            self.is_paused = not self.is_paused
            if self.is_paused:
                self.pause_event.set()
                self.update_status("Pemutaran dijeda", "orange")
            else:
                self.pause_event.clear()
                self.update_status(f"Sedang memutar: {os.path.basename(self.playlist[self.current_index])}", "green")

    def stop(self):
        if self.is_playing or self.is_paused:
            self.is_playing = False
            self.is_paused = False
            self.stop_event.set()
            self.pause_event.clear()
            self.update_status("Pemutaran dihentikan", "red")

    def remove_files(self):
        selected_indices = self.playlist_listbox.curselection()
        if not selected_indices:
            messagebox.showwarning("Peringatan", "Pilih dulu berkas yang ingin dihapus.")
            return

        for index in reversed(selected_indices):
            self.playlist.pop(index)
        self.update_playlist()


if __name__ == "__main__":
    root = tk.Tk()
    app = P3PlayerApp(root)
    root.mainloop()
