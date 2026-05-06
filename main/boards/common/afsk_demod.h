#pragma once

#include <vector>
#include <deque>
#include <string>
#include <memory>
#include <optional>
#include <cmath>
#include "wifi_manager.h"
#include "application.h"

// Konstanta pemrosesan sinyal audio untuk konfigurasi WiFi melalui suara
const size_t kAudioSampleRate = 6400;
const size_t kMarkFrequency = 1800;
const size_t kSpaceFrequency = 1500;
const size_t kBitRate = 100;
const size_t kWindowSize = 64;

namespace audio_wifi_config
{
    // Fungsi utama untuk menerima kredensial WiFi melalui sinyal audio
    void ReceiveWifiCredentialsFromAudio(Application *app, WifiManager *wifi_manager, Display *display, 
                                         size_t input_channels = 1);

    /**
     * Implementasi algoritma Goertzel untuk mendeteksi satu frekuensi
     * Dipakai untuk mendeteksi frekuensi audio tertentu pada proses demodulasi AFSK
     */
    class FrequencyDetector
    {
    private:
        float frequency_;              // Frekuensi target yang sudah dinormalisasi, yaitu f / fs
        size_t window_size_;           // Ukuran jendela analisis
        float frequency_bin_;          // Bin frekuensi
        float angular_frequency_;      // Frekuensi sudut
        float cos_coefficient_;        // Nilai cos(w)
        float sin_coefficient_;        // Nilai sin(w)
        float filter_coefficient_;     // Nilai 2 * cos(w)
        std::deque<float> state_buffer_;  // Buffer melingkar untuk menyimpan S[-1] dan S[-2]

    public:
        /**
         * Konstruktor
         * @param frequency Frekuensi yang telah dinormalisasi (f / fs)
         * @param window_size Ukuran jendela analisis
         */
        FrequencyDetector(float frequency, size_t window_size);

        /**
         * Reset status detektor
         */
        void Reset();

        /**
         * Proses satu sampel audio
         * @param sample Sampel audio masukan
         */
        void ProcessSample(float sample);

        /**
         * Hitung amplitudo saat ini
         * @return Nilai amplitudo
         */
        float GetAmplitude() const;
    };

    /**
     * Pemroses sinyal audio untuk mendeteksi pasangan frekuensi Mark/Space
     * Mengolah sinyal audio untuk mengekstrak data digital dengan demodulasi AFSK
     */
    class AudioSignalProcessor
    {
    private:
        std::deque<float> input_buffer_;             // Buffer sampel masukan
        size_t input_buffer_size_;                   // Ukuran buffer masukan = ukuran jendela
        size_t output_sample_count_;                 // Penghitung sampel keluaran
        size_t samples_per_bit_;                     // Ambang jumlah sampel per bit
        std::unique_ptr<FrequencyDetector> mark_detector_;   // Detektor frekuensi Mark
        std::unique_ptr<FrequencyDetector> space_detector_;  // Detektor frekuensi Space

    public:
        /**
         * Konstruktor
         * @param sample_rate Laju sampling audio
         * @param mark_frequency Frekuensi Mark untuk bit digital '1'
         * @param space_frequency Frekuensi Space untuk bit digital '0'
         * @param bit_rate Laju bit transmisi data
         * @param window_size Ukuran jendela analisis
         */
        AudioSignalProcessor(size_t sample_rate, size_t mark_frequency, size_t space_frequency,
                           size_t bit_rate, size_t window_size);

        /**
         * Proses sampel audio masukan
         * @param samples Vektor sampel audio masukan
         * @return Vektor nilai probabilitas Mark (0.0 sampai 1.0)
         */
        std::vector<float> ProcessAudioSamples(const std::vector<float> &samples);
    };

    /**
     * Status mesin keadaan untuk penerimaan data
     */
    enum class DataReceptionState
    {
        kInactive,  // Menunggu sinyal awal
        kWaiting,   // Awal sinyal terdeteksi, menunggu konfirmasi
        kReceiving  // Sedang aktif menerima data
    };

    /**
     * Buffer data untuk mengelola konversi audio menjadi data digital
     * Menangani alur lengkap dari sinyal audio hingga menjadi teks terdekode
     */
    class AudioDataBuffer
    {
    private:
        DataReceptionState current_state_;       // Status penerimaan saat ini
        std::deque<uint8_t> identifier_buffer_;  // Buffer untuk mendeteksi penanda awal/akhir
        size_t identifier_buffer_size_;          // Ukuran buffer penanda
        std::vector<uint8_t> bit_buffer_;        // Buffer untuk menyimpan aliran bit
        size_t max_bit_buffer_size_;             // Ukuran maksimum buffer bit
        const std::vector<uint8_t> start_of_transmission_;  // Penanda awal transmisi
        const std::vector<uint8_t> end_of_transmission_;    // Penanda akhir transmisi
        bool enable_checksum_validation_;       // Menentukan apakah checksum divalidasi

    public:
        std::optional<std::string> decoded_text; // Teks hasil dekode bila berhasil

        /**
         * Konstruktor bawaan dengan penanda awal dan akhir yang sudah ditetapkan
         */
        AudioDataBuffer();

        /**
         * Konstruktor dengan parameter khusus
         * @param max_byte_size Perkiraan ukuran data maksimum dalam byte
         * @param start_identifier Penanda awal transmisi
         * @param end_identifier Penanda akhir transmisi
         * @param enable_checksum Menentukan apakah validasi checksum diaktifkan
         */
        AudioDataBuffer(size_t max_byte_size, const std::vector<uint8_t> &start_identifier,
                      const std::vector<uint8_t> &end_identifier, bool enable_checksum = false);

        /**
         * Proses data probabilitas lalu coba dekode
         * @param probabilities Vektor probabilitas Mark
         * @param threshold Ambang keputusan untuk deteksi bit
         * @return true jika seluruh data berhasil diterima dan didekode
         */
        bool ProcessProbabilityData(const std::vector<float> &probabilities, float threshold = 0.5f);

        /**
         * Hitung checksum untuk teks ASCII
         * @param text Teks masukan
         * @return Nilai checksum (0-255)
         */
        static uint8_t CalculateChecksum(const std::string &text);

    private:
        /**
         * Ubah vektor bit menjadi vektor byte
         * @param bits Vektor bit masukan
         * @return Vektor byte hasil konversi
         */
        std::vector<uint8_t> ConvertBitsToBytes(const std::vector<uint8_t> &bits) const;

        /**
         * Bersihkan semua buffer dan setel ulang status
         */
        void ClearBuffers();
    };

    // Penanda bawaan untuk awal dan akhir transmisi
    extern const std::vector<uint8_t> kDefaultStartTransmissionPattern;
    extern const std::vector<uint8_t> kDefaultEndTransmissionPattern;
}
