import os
import sys
import tempfile
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

from PIL import Image

from LVGLImage import ColorFormat, CompressMethod, LVGLImage

HELP_TEXT = """Panduan penggunaan alat konversi gambar LVGL:

1. Tambah berkas:
   Klik tombol "Tambah Berkas" untuk memilih gambar yang ingin dikonversi. Impor banyak berkas didukung.

2. Hapus berkas:
   Pilih kotak "[ ]" pada daftar berkas. Setelah aktif akan berubah menjadi "[x]". Klik "Hapus Pilihan" untuk menghapus berkas yang dipilih.

3. Atur resolusi:
   Pilih resolusi yang dibutuhkan, misalnya 128x128. Sesuaikan dengan resolusi layar perangkat agar hasil tampilannya proporsional.

4. Format warna:
   Pilihan "Deteksi Otomatis" akan menyesuaikan format berdasarkan transparansi gambar. Gunakan pilihan ini bila tidak membutuhkan pengaturan manual.

5. Metode kompresi:
   Pilih NONE atau RLE. Jika tidak ada kebutuhan khusus, gunakan NONE.

6. Folder keluaran:
   Tentukan lokasi penyimpanan hasil konversi. Nilai bawaan menggunakan folder output di direktori program.

7. Mulai konversi:
   Klik "Konversi Semua Berkas" atau "Konversi Berkas Terpilih" untuk memulai proses.
"""


class ImageConverterApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Alat Konversi Gambar LVGL")
        self.root.geometry("750x650")

        # Inisialisasi variabel.
        self.output_dir = tk.StringVar(value=os.path.abspath("output"))
        self.resolution = tk.StringVar(value="128x128")
        self.color_format = tk.StringVar(value="Deteksi Otomatis")
        self.compress_method = tk.StringVar(value="NONE")

        # Buat komponen antarmuka.
        self.create_widgets()
        self.redirect_output()

    def create_widgets(self):
        # Bingkai pengaturan konversi.
        settings_frame = ttk.LabelFrame(self.root, text="Pengaturan Konversi")
        settings_frame.grid(row=0, column=0, padx=10, pady=5, sticky="ew")

        # Pengaturan resolusi.
        ttk.Label(settings_frame, text="Resolusi:").grid(row=0, column=0, padx=2)
        ttk.Combobox(
            settings_frame,
            textvariable=self.resolution,
            values=["512x512", "256x256", "128x128", "64x64", "32x32"],
            width=8,
        ).grid(row=0, column=1, padx=2)

        # Format warna.
        ttk.Label(settings_frame, text="Format Warna:").grid(row=0, column=2, padx=2)
        ttk.Combobox(
            settings_frame,
            textvariable=self.color_format,
            values=["Deteksi Otomatis", "RGB565", "RGB565A8"],
            width=16,
        ).grid(row=0, column=3, padx=2)

        # Metode kompresi.
        ttk.Label(settings_frame, text="Kompresi:").grid(row=0, column=4, padx=2)
        ttk.Combobox(
            settings_frame,
            textvariable=self.compress_method,
            values=["NONE", "RLE"],
            width=8,
        ).grid(row=0, column=5, padx=2)

        # Bingkai pengelolaan berkas.
        file_frame = ttk.LabelFrame(self.root, text="Pilih Berkas")
        file_frame.grid(row=1, column=0, padx=10, pady=5, sticky="nsew")

        # Tombol pengelolaan berkas.
        btn_frame = ttk.Frame(file_frame)
        btn_frame.pack(fill=tk.X, pady=2)
        ttk.Button(btn_frame, text="Tambah Berkas", command=self.select_files).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="Hapus Pilihan", command=self.remove_selected).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="Kosongkan Daftar", command=self.clear_files).pack(side=tk.LEFT, padx=2)

        # Daftar berkas menggunakan Treeview.
        self.tree = ttk.Treeview(file_frame, columns=("selected", "filename"), show="headings", height=10)
        self.tree.heading("selected", text="Pilih", anchor=tk.W)
        self.tree.heading("filename", text="Nama Berkas", anchor=tk.W)
        self.tree.column("selected", width=60, anchor=tk.W)
        self.tree.column("filename", width=600, anchor=tk.W)
        self.tree.pack(fill=tk.BOTH, expand=True)
        self.tree.bind("<ButtonRelease-1>", self.on_tree_click)

        # Folder keluaran.
        output_frame = ttk.LabelFrame(self.root, text="Folder Keluaran")
        output_frame.grid(row=2, column=0, padx=10, pady=5, sticky="ew")
        ttk.Entry(output_frame, textvariable=self.output_dir, width=60).pack(side=tk.LEFT, padx=5)
        ttk.Button(output_frame, text="Telusuri", command=self.select_output_dir).pack(side=tk.RIGHT, padx=5)

        # Tombol konversi dan bantuan.
        convert_frame = ttk.Frame(self.root)
        convert_frame.grid(row=3, column=0, padx=10, pady=10)
        ttk.Button(convert_frame, text="Konversi Semua Berkas", command=lambda: self.start_conversion(True)).pack(side=tk.LEFT, padx=5)
        ttk.Button(convert_frame, text="Konversi Berkas Terpilih", command=lambda: self.start_conversion(False)).pack(side=tk.LEFT, padx=5)
        ttk.Button(convert_frame, text="Bantuan", command=self.show_help).pack(side=tk.RIGHT, padx=5)

        # Area log beserta tombol bersihkan.
        log_frame = ttk.LabelFrame(self.root, text="Log")
        log_frame.grid(row=4, column=0, padx=10, pady=5, sticky="nsew")

        # Bingkai tombol pada area log.
        log_btn_frame = ttk.Frame(log_frame)
        log_btn_frame.pack(fill=tk.X, side=tk.BOTTOM)

        # Tombol pembersih log.
        ttk.Button(log_btn_frame, text="Kosongkan Log", command=self.clear_log).pack(side=tk.RIGHT, padx=5, pady=2)

        self.log_text = tk.Text(log_frame, height=15)
        self.log_text.pack(fill=tk.BOTH, expand=True)

        # Konfigurasi tata letak.
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(1, weight=1)
        self.root.rowconfigure(4, weight=1)

    def clear_log(self):
        """Kosongkan isi log."""
        self.log_text.delete(1.0, tk.END)

    def show_help(self):
        messagebox.showinfo("Bantuan", HELP_TEXT)

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

    def on_tree_click(self, event):
        region = self.tree.identify("region", event.x, event.y)
        if region == "cell":
            col = self.tree.identify_column(event.x)
            item = self.tree.identify_row(event.y)
            if col == "#1":  # Kolom pertama adalah kolom pilihan.
                current_val = self.tree.item(item, "values")[0]
                new_val = "[x]" if current_val == "[ ]" else "[ ]"
                self.tree.item(item, values=(new_val, self.tree.item(item, "values")[1]))

    def select_output_dir(self):
        path = filedialog.askdirectory()
        if path:
            self.output_dir.set(path)

    def select_files(self):
        files = filedialog.askopenfilenames(filetypes=[("Berkas Gambar", "*.png;*.jpg;*.jpeg;*.bmp;*.gif")])
        for file_path in files:
            self.tree.insert("", tk.END, values=("[ ]", os.path.basename(file_path)), tags=(file_path,))

    def remove_selected(self):
        to_remove = []
        for item in self.tree.get_children():
            if self.tree.item(item, "values")[0] == "[x]":
                to_remove.append(item)
        for item in reversed(to_remove):
            self.tree.delete(item)

    def clear_files(self):
        for item in self.tree.get_children():
            self.tree.delete(item)

    def start_conversion(self, convert_all):
        input_files = [
            self.tree.item(item, "tags")[0]
            for item in self.tree.get_children()
            if convert_all or self.tree.item(item, "values")[0] == "[x]"
        ]

        if not input_files:
            msg = "Tidak ada berkas yang bisa dikonversi." if convert_all else "Belum ada berkas yang dipilih."
            messagebox.showwarning("Peringatan", msg)
            return

        os.makedirs(self.output_dir.get(), exist_ok=True)

        # Urai parameter konversi.
        width, height = map(int, self.resolution.get().split("x"))
        compress = CompressMethod.RLE if self.compress_method.get() == "RLE" else CompressMethod.NONE

        # Jalankan konversi.
        self.convert_images(input_files, width, height, compress)

    def convert_images(self, input_files, width, height, compress):
        success_count = 0
        total_files = len(input_files)

        for file_path in input_files:
            try:
                print(f"Sedang memproses: {os.path.basename(file_path)}")

                with Image.open(file_path) as image:
                    # Ubah ukuran gambar sesuai resolusi target.
                    image = image.resize((width, height), Image.Resampling.LANCZOS)

                    # Tentukan format warna yang dipakai.
                    color_format_str = self.color_format.get()
                    if color_format_str == "Deteksi Otomatis":
                        # Periksa apakah gambar memiliki kanal transparansi.
                        has_alpha = image.mode in ("RGBA", "LA") or (image.mode == "P" and "transparency" in image.info)
                        if has_alpha:
                            image = image.convert("RGBA")
                            color_format = ColorFormat.RGB565A8
                        else:
                            image = image.convert("RGB")
                            color_format = ColorFormat.RGB565
                    else:
                        if color_format_str == "RGB565A8":
                            image = image.convert("RGBA")
                            color_format = ColorFormat.RGB565A8
                        else:
                            image = image.convert("RGB")
                            color_format = ColorFormat.RGB565

                    # Simpan gambar hasil penyesuaian resolusi.
                    base_name = os.path.splitext(os.path.basename(file_path))[0]
                    output_image_path = os.path.join(self.output_dir.get(), f"{base_name}_{width}x{height}.png")
                    image.save(output_image_path, "PNG")

                    # Buat berkas sementara untuk sumber konversi LVGL.
                    with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmpfile:
                        temp_path = tmpfile.name
                        image.save(temp_path, "PNG")

                    # Ubah menjadi array C untuk LVGL.
                    lvgl_img = LVGLImage().from_png(temp_path, cf=color_format)
                    output_c_path = os.path.join(self.output_dir.get(), f"{base_name}.c")
                    lvgl_img.to_c_array(output_c_path, compress=compress)

                    success_count += 1
                    os.unlink(temp_path)
                    print(f"Berhasil dikonversi: {base_name}.c\n")

            except Exception as exc:
                print(f"Konversi gagal: {str(exc)}\n")

        print(f"Konversi selesai. Berhasil {success_count}/{total_files} berkas.\n")


if __name__ == "__main__":
    root = tk.Tk()
    app = ImageConverterApp(root)
    root.mainloop()
