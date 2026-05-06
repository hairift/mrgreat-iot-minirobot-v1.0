import asyncio
import sys
import wave
from collections import deque

import matplotlib
import numpy as np
import qasync
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qtagg import NavigationToolbar2QT as NavigationToolbar  # noqa: F401
from matplotlib.figure import Figure
from PyQt6.QtCore import QTimer
from PyQt6.QtWidgets import (
    QApplication,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMainWindow,
    QPushButton,
    QTextEdit,
    QVBoxLayout,
    QWidget,
)

from demod import RealTimeAFSKDecoder

matplotlib.use("qtagg")


class UDPServerProtocol(asyncio.DatagramProtocol):
    """Protokol server UDP untuk menerima data audio."""

    def __init__(self, data_queue):
        self.client_address = None
        self.data_queue: deque = data_queue

    def connection_made(self, transport):
        self.transport = transport

    def datagram_received(self, data, addr):
        # Simpan alamat klien pertama yang terhubung.
        if self.client_address is None:
            self.client_address = addr
            print(f"Menerima koneksi dari {addr}")

        # Hanya terima data dari klien yang sudah dikenali.
        if addr == self.client_address:
            self.data_queue.extend(data)
        else:
            print(f"Mengabaikan data dari alamat tak dikenal {addr}")


class MatplotlibWidget(QWidget):
    """Widget untuk menampilkan gelombang audio dan spektrum frekuensi."""

    def __init__(self, parent=None):
        super().__init__(parent)

        # Buat Figure dan kanvas Matplotlib.
        self.figure = Figure()
        self.canvas = FigureCanvas(self.figure)

        # Bilah navigasi sengaja tidak dipakai, tetapi impor dipertahankan.
        self.toolbar = None

        # Susun tata letak widget.
        layout = QVBoxLayout()
        layout.addWidget(self.toolbar)
        layout.addWidget(self.canvas)
        self.setLayout(layout)

        # Parameter dasar audio.
        self.freq = 16000  # Frekuensi sampling.
        self.time_window = 20  # Jendela tampilan dalam detik.
        self.wave_data = deque(maxlen=self.freq * self.time_window * 2)
        self.signals = deque(maxlen=self.freq * self.time_window)

        # Buat dua subplot: domain waktu dan domain frekuensi.
        self.ax1 = self.figure.add_subplot(2, 1, 1)
        self.ax2 = self.figure.add_subplot(2, 1, 2)

        self.ax1.set_title("Gelombang Audio Waktu Nyata")
        self.ax1.set_xlabel("Indeks Sampel")
        self.ax1.set_ylabel("Amplitudo")
        self.line_time, = self.ax1.plot([], [])
        self.ax1.grid(True, alpha=0.3)

        self.ax2.set_title("Spektrum Frekuensi Waktu Nyata")
        self.ax2.set_xlabel("Frekuensi (Hz)")
        self.ax2.set_ylabel("Magnitudo")
        self.line_freq, = self.ax2.plot([], [])
        self.ax2.grid(True, alpha=0.3)

        self.figure.tight_layout()

        # Pewaktu dipakai untuk memperbarui grafik secara berkala.
        self.timer = QTimer(self)
        self.timer.setInterval(100)
        self.timer.timeout.connect(self.update_plot)

        # Inisialisasi dekoder AFSK.
        self.decoder = RealTimeAFSKDecoder(
            f_sample=self.freq,
            mark_freq=1800,
            space_freq=1500,
            bitrate=100,
            s_goertzel=9,
            threshold=0.5,
        )

        # Pemanggil ini diisi oleh jendela utama.
        self.decode_callback = None

    def start_plotting(self):
        """Memulai pembaruan grafik."""
        self.timer.start()

    def stop_plotting(self):
        """Menghentikan pembaruan grafik."""
        self.timer.stop()

    def update_plot(self):
        """Memperbarui data visualisasi dan hasil dekode."""
        if len(self.wave_data) >= 2:
            # Dekode audio yang baru masuk.
            even = len(self.wave_data) // 2 * 2
            print(f"Panjang wave_data: {len(self.wave_data)}")
            drained = [self.wave_data.popleft() for _ in range(even)]
            signal = np.frombuffer(bytearray(drained), dtype="<i2") / 32768
            decoded_text_new = self.decoder.process_audio(signal)
            if decoded_text_new and self.decode_callback:
                self.decode_callback(decoded_text_new)
            self.signals.extend(signal.tolist())

        if len(self.signals) > 0:
            # Tampilkan hanya data terbaru agar grafik tetap jelas.
            signal = np.array(self.signals)
            max_samples = min(len(signal), self.freq * self.time_window)
            if len(signal) > max_samples:
                signal = signal[-max_samples:]

            # Perbarui grafik domain waktu.
            x_axis = np.arange(len(signal))
            self.line_time.set_data(x_axis, signal)

            # Atur ulang sumbu domain waktu secara otomatis.
            if len(signal) > 0:
                self.ax1.set_xlim(0, len(signal))
                y_min, y_max = np.min(signal), np.max(signal)
                if y_min != y_max:
                    margin = (y_max - y_min) * 0.1
                    self.ax1.set_ylim(y_min - margin, y_max + margin)
                else:
                    self.ax1.set_ylim(-1, 1)

            # Hitung lalu tampilkan spektrum frekuensi.
            if len(signal) > 1:
                fft_signal = np.abs(np.fft.fft(signal))
                frequencies = np.fft.fftfreq(len(signal), 1 / self.freq)

                positive_freq_idx = frequencies >= 0
                freq_positive = frequencies[positive_freq_idx]
                fft_positive = fft_signal[positive_freq_idx]

                self.line_freq.set_data(freq_positive, fft_positive)

                # Batasi sumbu frekuensi agar tetap mudah dibaca.
                if len(fft_positive) > 0:
                    max_freq_show = min(4000, self.freq // 2)
                    freq_mask = freq_positive <= max_freq_show
                    if np.any(freq_mask):
                        self.ax2.set_xlim(0, max_freq_show)
                        fft_masked = fft_positive[freq_mask]
                        if len(fft_masked) > 0:
                            fft_max = np.max(fft_masked)
                            if fft_max > 0:
                                self.ax2.set_ylim(0, fft_max * 1.1)
                            else:
                                self.ax2.set_ylim(0, 1)

            self.canvas.draw()


class MainWindow(QMainWindow):
    """Jendela utama untuk alat pemantauan akustik."""

    def __init__(self):
        super().__init__()
        self.setWindowTitle("Pemeriksaan Akustik")
        self.setGeometry(100, 100, 1000, 800)

        # Buat widget utama.
        main_widget = QWidget()
        self.setCentralWidget(main_widget)

        # Tata letak utama jendela.
        main_layout = QVBoxLayout(main_widget)

        # Area grafik.
        self.matplotlib_widget = MatplotlibWidget()
        main_layout.addWidget(self.matplotlib_widget)

        # Panel kontrol.
        control_panel = QWidget()
        control_layout = QHBoxLayout(control_panel)

        control_layout.addWidget(QLabel("Alamat dengar:"))
        self.address_input = QLineEdit("0.0.0.0")
        self.address_input.setFixedWidth(120)
        control_layout.addWidget(self.address_input)

        control_layout.addWidget(QLabel("Port:"))
        self.port_input = QLineEdit("8000")
        self.port_input.setFixedWidth(80)
        control_layout.addWidget(self.port_input)

        self.listen_button = QPushButton("Mulai dengar")
        self.listen_button.clicked.connect(self.toggle_listening)
        control_layout.addWidget(self.listen_button)

        self.status_label = QLabel("Status: belum terhubung")
        control_layout.addWidget(self.status_label)

        self.data_label = QLabel("Data diterima: 0 byte")
        control_layout.addWidget(self.data_label)

        self.save_button = QPushButton("Simpan audio")
        self.save_button.clicked.connect(self.save_audio)
        self.save_button.setEnabled(False)
        control_layout.addWidget(self.save_button)

        control_layout.addStretch()
        main_layout.addWidget(control_panel)

        # Panel hasil dekode.
        decode_panel = QWidget()
        decode_layout = QVBoxLayout(decode_panel)

        decode_title = QLabel("Hasil dekode AFSK waktu nyata:")
        decode_title.setStyleSheet("font-weight: bold; font-size: 14px;")
        decode_layout.addWidget(decode_title)

        self.decode_text = QTextEdit()
        self.decode_text.setMaximumHeight(150)
        self.decode_text.setReadOnly(True)
        self.decode_text.setStyleSheet("font-family: 'Courier New', monospace; font-size: 12px;")
        decode_layout.addWidget(self.decode_text)

        decode_control_layout = QHBoxLayout()

        self.clear_decode_button = QPushButton("Bersihkan hasil")
        self.clear_decode_button.clicked.connect(self.clear_decode_text)
        decode_control_layout.addWidget(self.clear_decode_button)

        self.decode_stats_label = QLabel("Statistik dekode: 0 bit, 0 karakter")
        decode_control_layout.addWidget(self.decode_stats_label)

        decode_control_layout.addStretch()
        decode_layout.addLayout(decode_control_layout)

        main_layout.addWidget(decode_panel)

        # Kaitkan callback hasil dekode.
        self.matplotlib_widget.decode_callback = self.on_decode_text

        # Properti terkait UDP.
        self.udp_transport = None
        self.is_listening = False

        # Pewaktu statistik.
        self.stats_timer = QTimer(self)
        self.stats_timer.setInterval(1000)
        self.stats_timer.timeout.connect(self.update_stats)

    def on_decode_text(self, new_text: str):
        """Menambahkan teks baru ke panel hasil dekode."""
        if new_text:
            current_text = self.decode_text.toPlainText()
            updated_text = current_text + new_text

            # Batasi panjang tampilan agar tidak tumbuh tanpa batas.
            if len(updated_text) > 1000:
                updated_text = updated_text[-1000:]

            self.decode_text.setPlainText(updated_text)

            # Geser tampilan ke bagian paling akhir.
            cursor = self.decode_text.textCursor()
            cursor.movePosition(cursor.MoveOperation.End)
            self.decode_text.setTextCursor(cursor)

    def clear_decode_text(self):
        """Mengosongkan panel dekode dan mereset statistik dekoder."""
        self.decode_text.clear()
        if hasattr(self.matplotlib_widget, "decoder"):
            self.matplotlib_widget.decoder.clear()
        self.decode_stats_label.setText("Statistik dekode: 0 bit, 0 karakter")

    def update_decode_stats(self):
        """Memperbarui label statistik hasil dekode."""
        if hasattr(self.matplotlib_widget, "decoder"):
            stats = self.matplotlib_widget.decoder.get_stats()
            stats_text = (
                f"Awalan: {stats['prelude_bits']}, "
                f"karakter diterima: {stats['total_chars']}, "
                f"penyangga: {stats['buffer_bits']} bit, "
                f"status: {stats['state']}"
            )
            self.decode_stats_label.setText(stats_text)

    def toggle_listening(self):
        """Mengganti keadaan antara mulai dan berhenti mendengar."""
        if not self.is_listening:
            self.start_listening()
        else:
            self.stop_listening()

    async def start_listening_async(self):
        """Menjalankan server UDP secara asinkron."""
        try:
            address = self.address_input.text().strip()
            port = int(self.port_input.text().strip())

            loop = asyncio.get_running_loop()
            self.udp_transport, _protocol = await loop.create_datagram_endpoint(
                lambda: UDPServerProtocol(self.matplotlib_widget.wave_data),
                local_addr=(address, port),
            )

            self.status_label.setText(f"Status: sedang mendengar ({address}:{port})")
            print(f"Server UDP aktif pada {address}:{port}")

        except Exception as error:
            self.status_label.setText(f"Status: gagal mulai - {error}")
            print(f"Server UDP gagal dijalankan: {error}")
            self.is_listening = False
            self.listen_button.setText("Mulai dengar")
            self.address_input.setEnabled(True)
            self.port_input.setEnabled(True)

    def start_listening(self):
        """Memulai proses pemantauan UDP."""
        try:
            int(self.port_input.text().strip())
        except ValueError:
            self.status_label.setText("Status: port harus berupa angka")
            return

        self.is_listening = True
        self.listen_button.setText("Hentikan dengar")
        self.address_input.setEnabled(False)
        self.port_input.setEnabled(False)
        self.save_button.setEnabled(True)

        # Kosongkan penyangga data sebelum sesi baru dimulai.
        self.matplotlib_widget.wave_data.clear()

        # Mulai pembaruan grafik dan statistik.
        self.matplotlib_widget.start_plotting()
        self.stats_timer.start()

        loop = asyncio.get_event_loop()
        loop.create_task(self.start_listening_async())

    def stop_listening(self):
        """Menghentikan pemantauan UDP."""
        self.is_listening = False
        self.listen_button.setText("Mulai dengar")
        self.address_input.setEnabled(True)
        self.port_input.setEnabled(True)

        # Tutup transport UDP bila ada.
        if self.udp_transport:
            self.udp_transport.close()
            self.udp_transport = None

        # Hentikan pembaruan visual dan kosongkan penyangga.
        self.matplotlib_widget.stop_plotting()
        self.matplotlib_widget.wave_data.clear()
        self.stats_timer.stop()

        self.status_label.setText("Status: dihentikan")

    def update_stats(self):
        """Memperbarui statistik data dan statistik dekode."""
        data_size = len(self.matplotlib_widget.signals)
        self.data_label.setText(f"Data diterima: {data_size} sampel")
        self.update_decode_stats()

    def save_audio(self):
        """Menyimpan audio yang sudah diterima ke berkas WAV."""
        if len(self.matplotlib_widget.signals) > 0:
            try:
                signal_data = np.array(self.matplotlib_widget.signals)

                with wave.open("received_audio.wav", "wb") as wave_file:
                    wave_file.setnchannels(1)  # Mono.
                    wave_file.setsampwidth(2)  # Lebar sampel 2 byte.
                    wave_file.setframerate(self.matplotlib_widget.freq)
                    wave_file.writeframes(signal_data.tobytes())

                self.status_label.setText("Status: audio tersimpan sebagai received_audio.wav")
                print("Audio tersimpan sebagai received_audio.wav")

            except Exception as error:
                self.status_label.setText(f"Status: gagal menyimpan - {error}")
                print(f"Gagal menyimpan audio: {error}")
        else:
            self.status_label.setText("Status: data belum cukup untuk disimpan")


async def main():
    """Menjalankan aplikasi GUI secara asinkron."""
    app = QApplication(sys.argv)

    # Gunakan event loop Qt yang kompatibel dengan asyncio.
    loop = qasync.QEventLoop(app)
    asyncio.set_event_loop(loop)

    window = MainWindow()
    window.show()

    try:
        with loop:
            await loop.run_forever()
    except KeyboardInterrupt:
        print("Program dihentikan oleh pengguna")
    finally:
        # Pastikan transport UDP ditutup saat aplikasi berakhir.
        if window.udp_transport:
            window.udp_transport.close()
