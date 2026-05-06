#ifndef OGG_DEMUXER_H_
#define OGG_DEMUXER_H_

#include <functional>
#include <cstdint>
#include <cstring>
#include <vector>

class OggDemuxer {
private:
    enum ParseState : int8_t {
        FIND_PAGE,
        PARSE_HEADER,
        PARSE_SEGMENTS,
        PARSE_DATA
    };

    struct Opus_t {
        bool head_seen{false};
        bool tags_seen{false};
        int sample_rate{48000};
    };

    // Gunakan penyangga berukuran tetap agar tidak perlu alokasi dinamis
    struct context_t {
        bool packet_continued{false};   // Menandai apakah paket saat ini melintasi beberapa segmen
        uint8_t header[27];             // Header halaman Ogg
        uint8_t seg_table[255];         // Tabel segmen yang sedang diproses
        uint8_t packet_buf[8192];       // Buffer paket berukuran 8 KB
        size_t packet_len = 0;          // Total data yang sudah terkumpul di buffer paket
        size_t seg_count = 0;           // Jumlah segmen pada halaman saat ini
        size_t seg_index = 0;           // Indeks segmen yang sedang diproses
        size_t data_offset = 0;         // Jumlah byte yang sudah dibaca pada tahap saat ini
        size_t bytes_needed = 0;        // Jumlah byte yang masih diperlukan pada tahap saat ini
        size_t seg_remaining = 0;       // Sisa byte yang harus dibaca dari segmen saat ini
        size_t body_size = 0;           // Total ukuran badan data halaman
        size_t body_offset = 0;         // Jumlah byte badan data yang sudah dibaca
    };

public:
    OggDemuxer() {
        Reset();
    }

    void Reset();

    size_t Process(const uint8_t* data, size_t size);

    /// @brief Atur fungsi panggil balik setelah proses demuks selesai
    /// @param on_demuxer_finished Fungsi yang menerima data audio, laju sampel, dan panjang data
    void OnDemuxerFinished(std::function<void(const uint8_t* data, int sample_rate, size_t len)> on_demuxer_finished) {
        on_demuxer_finished_ = on_demuxer_finished;
    }

private:
    ParseState state_ = ParseState::FIND_PAGE;
    context_t ctx_;
    Opus_t opus_info_;
    std::function<void(const uint8_t*, int, size_t)> on_demuxer_finished_;
};

#endif
