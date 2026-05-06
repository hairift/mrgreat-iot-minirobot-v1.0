import os
import sys
import threading
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

from convert_audio_to_p3 import encode_audio_to_opus
from convert_p3_to_audio import decode_p3_to_audio


class AudioConverterApp:
    def __init__(self, master):
        self.master = master
        master.title("Alat Konversi Batch Audio/P3")
        master.geometry("680x600")  # Sesuaikan tinggi jendela.

        # Inisialisasi variabel.
        self.mode = tk.StringVar(value="audio_to_p3")
        self.output_dir = tk.StringVar()
        self.output_dir.set(os.path.abspath("output"))
        self.enable_loudnorm = tk.BooleanVar(value=True)
        self.target_lufs = tk.DoubleVar(value=-16.0)

        # Buat komponen antarmuka.
        self.create_widgets()
        self.redirect_output()

    def create_widgets(self):
        # Pilihan mode konversi.
        mode_frame = ttk.LabelFrame(self.master, text="Mode Konversi")
        mode_frame.grid(row=0, column=0, padx=10, pady=5, sticky="ew")

        ttk.Radiobutton(
            mode_frame,
            text="Audio ke P3",
            variable=self.mode,
            value="audio_to_p3",
            command=self.toggle_settings,
            width=14,
        ).grid(row=0, column=0, padx=5)
        ttk.Radiobutton(
            mode_frame,
            text="P3 ke Audio",
            variable=self.mode,
            value="p3_to_audio",
            command=self.toggle_settings,
            width=14,
        ).grid(row=0, column=1, padx=5)

        # Pengaturan loudness.
        self.loudnorm_frame = ttk.Frame(self.master)
        self.loudnorm_frame.grid(row=1, column=0, padx=10, pady=5, sticky="ew")

        ttk.Checkbutton(
            self.loudnorm_frame,
            text="Aktifkan Penyesuaian Loudness",
            variable=self.enable_loudnorm,
            width=28,
        ).grid(row=0, column=0, padx=2)
        ttk.Entry(self.loudnorm_frame, textvariable=self.target_lufs, width=6).grid(row=0, column=1, padx=2)
        ttk.Label(self.loudnorm_frame, text="LUFS").grid(row=0, column=2, padx=2)

        # Pilihan berkas masukan.
        file_frame = ttk.LabelFrame(self.master, text="Berkas Masukan")
        file_frame.grid(row=2, column=0, padx=10, pady=5, sticky="nsew")

        # Tombol pengelolaan berkas.
        ttk.Button(file_frame, text="Pilih Berkas", command=self.select_files, width=14).grid(row=0, column=0, padx=5, pady=2)
        ttk.Button(file_frame, text="Hapus Pilihan", command=self.remove_selected, width=14).grid(row=0, column=1, padx=5, pady=2)
        ttk.Button(file_frame, text="Kosongkan Daftar", command=self.clear_files, width=16).grid(row=0, column=2, padx=5, pady=2)

        # Daftar berkas menggunakan Treeview.
        self.tree = ttk.Treeview(file_frame, columns=("selected", "filename"), show="headings", height=8)
        self.tree.heading("selected", text="Pilih", anchor=tk.W)
        self.tree.heading("filename", text="Nama Berkas", anchor=tk.W)
        self.tree.column("selected", width=60, anchor=tk.W)
        self.tree.column("filename", width=600, anchor=tk.W)
        self.tree.grid(row=1, column=0, columnspan=3, sticky="nsew", padx=5, pady=2)
        self.tree.bind("<ButtonRelease-1>", self.on_tree_click)

        # Folder keluaran.
        output_frame = ttk.LabelFrame(self.master, text="Folder Keluaran")
        output_frame.grid(row=3, column=0, padx=10, pady=5, sticky="ew")

        ttk.Entry(output_frame, textvariable=self.output_dir, width=60).grid(row=0, column=0, padx=5, sticky="ew")
        ttk.Button(output_frame, text="Telusuri", command=self.select_output_dir, width=10).grid(row=0, column=1, padx=5)

        # Area tombol konversi.
        button_frame = ttk.Frame(self.master)
        button_frame.grid(row=4, column=0, padx=10, pady=10, sticky="ew")

        ttk.Button(button_frame, text="Konversi Semua Berkas", command=lambda: self.start_conversion(True), width=22).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Konversi Berkas Terpilih", command=lambda: self.start_conversion(False), width=22).pack(side=tk.LEFT, padx=5)

        # Area log.
        log_frame = ttk.LabelFrame(self.master, text="Log")
        log_frame.grid(row=5, column=0, padx=10, pady=5, sticky="nsew")

        self.log_text = tk.Text(log_frame, height=14, width=80)
        self.log_text.pack(fill=tk.BOTH, expand=True)

        # Atur bobot tata letak.
        self.master.columnconfigure(0, weight=1)
        self.master.rowconfigure(2, weight=1)
        self.master.rowconfigure(5, weight=3)
        file_frame.columnconfigure(0, weight=1)
        file_frame.rowconfigure(1, weight=1)

    def toggle_settings(self):
        if self.mode.get() == "audio_to_p3":
            self.loudnorm_frame.grid()
        else:
            self.loudnorm_frame.grid_remove()

    def select_files(self):
        file_types = [
            ("Berkas Audio", "*.wav *.mp3 *.ogg *.flac") if self.mode.get() == "audio_to_p3" else ("Berkas P3", "*.p3")
        ]

        files = filedialog.askopenfilenames(filetypes=file_types)
        for file_path in files:
            self.tree.insert("", tk.END, values=("[ ]", os.path.basename(file_path)), tags=(file_path,))

    def on_tree_click(self, event):
        """Tangani klik pada kotak pilih."""
        region = self.tree.identify("region", event.x, event.y)
        if region == "cell":
            col = self.tree.identify_column(event.x)
            item = self.tree.identify_row(event.y)
            if col == "#1":  # Kolom pertama adalah kolom pilihan.
                current_val = self.tree.item(item, "values")[0]
                new_val = "[x]" if current_val == "[ ]" else "[ ]"
                self.tree.item(item, values=(new_val, self.tree.item(item, "values")[1]))

    def remove_selected(self):
        """Hapus berkas yang dipilih."""
        to_remove = []
        for item in self.tree.get_children():
            if self.tree.item(item, "values")[0] == "[x]":
                to_remove.append(item)
        for item in reversed(to_remove):
            self.tree.delete(item)

    def clear_files(self):
        """Kosongkan seluruh daftar berkas."""
        for item in self.tree.get_children():
            self.tree.delete(item)

    def select_output_dir(self):
        path = filedialog.askdirectory()
        if path:
            self.output_dir.set(path)

    def redirect_output(self):
        class StdoutRedirector:
            def __init__(self, text_widget):
                self.text_widget = text_widget
                self.original_stdout = sys.stdout

            def write(self, message):
                self.text_widget.insert(tk.END, message)
                self.text_widget.see(tk.END)
                self.original_stdout.write(message)

            def flush(self):
                self.original_stdout.flush()

        sys.stdout = StdoutRedirector(self.log_text)

    def start_conversion(self, convert_all):
        """Mulai proses konversi."""
        input_files = []
        for item in self.tree.get_children():
            if convert_all or self.tree.item(item, "values")[0] == "[x]":
                input_files.append(self.tree.item(item, "tags")[0])

        if not input_files:
            msg = "Tidak ada berkas yang bisa dikonversi." if convert_all else "Belum ada berkas yang dipilih."
            messagebox.showwarning("Peringatan", msg)
            return

        os.makedirs(self.output_dir.get(), exist_ok=True)

        try:
            if self.mode.get() == "audio_to_p3":
                target_lufs = self.target_lufs.get() if self.enable_loudnorm.get() else None
                thread = threading.Thread(target=self.convert_audio_to_p3, args=(target_lufs, input_files))
            else:
                thread = threading.Thread(target=self.convert_p3_to_audio, args=(input_files,))

            thread.start()
        except Exception as exc:
            print(f"Gagal menyiapkan konversi: {str(exc)}")

    def convert_audio_to_p3(self, target_lufs, input_files):
        """Logika konversi dari audio ke P3."""
        for input_path in input_files:
            try:
                filename = os.path.basename(input_path)
                base_name = os.path.splitext(filename)[0]
                output_path = os.path.join(self.output_dir.get(), f"{base_name}.p3")

                print(f"Sedang mengonversi: {filename}")
                encode_audio_to_opus(input_path, output_path, target_lufs)
                print(f"Konversi berhasil: {filename}\n")
            except Exception as exc:
                print(f"Konversi gagal: {str(exc)}\n")

    def convert_p3_to_audio(self, input_files):
        """Logika konversi dari P3 ke audio."""
        for input_path in input_files:
            try:
                filename = os.path.basename(input_path)
                base_name = os.path.splitext(filename)[0]
                output_path = os.path.join(self.output_dir.get(), f"{base_name}.wav")

                print(f"Sedang mengonversi: {filename}")
                decode_p3_to_audio(input_path, output_path)
                print(f"Konversi berhasil: {filename}\n")
            except Exception as exc:
                print(f"Konversi gagal: {str(exc)}\n")


if __name__ == "__main__":
    root = tk.Tk()
    app = AudioConverterApp(root)
    root.mainloop()
