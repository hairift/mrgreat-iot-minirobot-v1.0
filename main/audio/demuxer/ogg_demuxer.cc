#include "ogg_demuxer.h"
#include "esp_log.h"

#define TAG "OggDemuxer"

/// @brief Setel ulang status demukser
void OggDemuxer::Reset()
{
    opus_info_ = {
        .head_seen = false,
        .tags_seen = false,
        .sample_rate = 48000
    };

    state_ = ParseState::FIND_PAGE;
    ctx_.packet_len = 0;
    ctx_.seg_count = 0;
    ctx_.seg_index = 0;
    ctx_.data_offset = 0;
    ctx_.bytes_needed = 4;          // Membutuhkan 4 byte untuk penanda "OggS"
    ctx_.seg_remaining = 0;
    ctx_.body_size = 0;
    ctx_.body_offset = 0;
    ctx_.packet_continued = false;

    // Kosongkan data penyangga internal
    memset(ctx_.header, 0, sizeof(ctx_.header));
    memset(ctx_.seg_table, 0, sizeof(ctx_.seg_table));
    memset(ctx_.packet_buf, 0, sizeof(ctx_.packet_buf));
}

/// @brief Proses satu blok data masukan
/// @param data Data masukan
/// @param size Ukuran data masukan
/// @return Jumlah byte yang berhasil diproses
size_t OggDemuxer::Process(const uint8_t* data, size_t size)
{
    size_t processed = 0;  // Jumlah byte yang sudah diproses

    while (processed < size) {
        switch (state_) {
        case ParseState::FIND_PAGE: {
            // Cari penanda awal halaman "OggS"
            if (ctx_.bytes_needed < 4) {
                // Tangani kecocokan "OggS" yang terpotong antarblok data
                size_t to_copy = std::min(size - processed, ctx_.bytes_needed);
                memcpy(ctx_.header + (4 - ctx_.bytes_needed), data + processed, to_copy);

                processed += to_copy;
                ctx_.bytes_needed -= to_copy;

                if (ctx_.bytes_needed == 0) {
                    // Periksa apakah header cocok dengan "OggS"
                    if (memcmp(ctx_.header, "OggS", 4) == 0) {
                        state_ = ParseState::PARSE_HEADER;
                        ctx_.data_offset = 4;
                        ctx_.bytes_needed = 27 - 4;  // Masih perlu 23 byte untuk melengkapi header
                    } else {
                        // Jika gagal cocok, geser 1 byte lalu coba lagi
                        memmove(ctx_.header, ctx_.header + 1, 3);
                        ctx_.bytes_needed = 1;
                    }
                } else {
                    // Data belum cukup, tunggu blok berikutnya
                    return processed;
                }
            } else if (ctx_.bytes_needed == 4) {
                // Cari "OggS" utuh di dalam blok data yang tersedia
                bool found = false;
                size_t i = 0;
                size_t remaining = size - processed;

                // Lakukan pencarian pola "OggS"
                for (; i + 4 <= remaining; i++) {
                    if (memcmp(data + processed + i, "OggS", 4) == 0) {
                        found = true;
                        break;
                    }
                }

                if (found) {
                    // Lompat ke posisi awal "OggS" yang ditemukan
                    processed += i;

                    // Tidak perlu menyalin "OggS" ke penyangga header lagi
                    processed += 4;

                    state_ = ParseState::PARSE_HEADER;
                    ctx_.data_offset = 4;
                    ctx_.bytes_needed = 27 - 4;  // Masih perlu 23 byte
                } else {
                    // Tidak ditemukan "OggS" utuh, simpan kemungkinan kecocokan parsial
                    size_t partial_len = remaining - i;
                    if (partial_len > 0) {
                        memcpy(ctx_.header, data + processed + i, partial_len);
                        ctx_.bytes_needed = 4 - partial_len;
                        processed += i + partial_len;
                    } else {
                        processed += i;  // Semua byte sudah diperiksa
                    }
                    return processed;  // Kembalikan jumlah byte yang sudah diproses
                }
            } else {
                ESP_LOGE(TAG, "OggDemuxer run in error state: bytes_needed=%zu", ctx_.bytes_needed);
                Reset();
                return processed;
            }
            break;
        }

        case ParseState::PARSE_HEADER: {
            size_t available = size - processed;

            if (available < ctx_.bytes_needed) {
                // Data belum cukup, salin bagian yang tersedia dulu
                memcpy(ctx_.header + ctx_.data_offset, data + processed, available);

                ctx_.data_offset += available;
                ctx_.bytes_needed -= available;
                processed += available;
                return processed;  // Tunggu data berikutnya
            } else {
                // Data cukup untuk melengkapi header
                size_t to_copy = ctx_.bytes_needed;
                memcpy(ctx_.header + ctx_.data_offset, data + processed, to_copy);

                processed += to_copy;
                ctx_.data_offset += to_copy;
                ctx_.bytes_needed = 0;

                // Periksa validitas header
                if (ctx_.header[4] != 0) {
                    ESP_LOGE(TAG, "Versi Ogg tidak valid: %d", ctx_.header[4]);
                    state_ = ParseState::FIND_PAGE;
                    ctx_.bytes_needed = 4;
                    ctx_.data_offset = 0;
                    break;
                }

                ctx_.seg_count = ctx_.header[26];
                if (ctx_.seg_count > 0 && ctx_.seg_count <= 255) {
                    state_ = ParseState::PARSE_SEGMENTS;
                    ctx_.bytes_needed = ctx_.seg_count;
                    ctx_.data_offset = 0;
                } else if (ctx_.seg_count == 0) {
                    // Jika tidak ada segmen, langsung cari halaman berikutnya
                    state_ = ParseState::FIND_PAGE;
                    ctx_.bytes_needed = 4;
                    ctx_.data_offset = 0;
                } else {
                    ESP_LOGE(TAG, "Jumlah segmen tidak valid: %u", ctx_.seg_count);
                    state_ = ParseState::FIND_PAGE;
                    ctx_.bytes_needed = 4;
                    ctx_.data_offset = 0;
                }
            }
            break;
        }

        case ParseState::PARSE_SEGMENTS: {
            size_t available = size - processed;

            if (available < ctx_.bytes_needed) {
                memcpy(ctx_.seg_table + ctx_.data_offset, data + processed, available);

                ctx_.data_offset += available;
                ctx_.bytes_needed -= available;
                processed += available;
                return processed;  // Tunggu data berikutnya
            } else {
                size_t to_copy = ctx_.bytes_needed;
                memcpy(ctx_.seg_table + ctx_.data_offset, data + processed, to_copy);

                processed += to_copy;
                ctx_.data_offset += to_copy;
                ctx_.bytes_needed = 0;

                state_ = ParseState::PARSE_DATA;
                ctx_.seg_index = 0;
                ctx_.data_offset = 0;

                // Hitung total ukuran badan data pada halaman ini
                ctx_.body_size = 0;
                for (size_t i = 0; i < ctx_.seg_count; ++i) {
                    ctx_.body_size += ctx_.seg_table[i];
                }
                ctx_.body_offset = 0;
                ctx_.seg_remaining = 0;
            }
            break;
        }

        case ParseState::PARSE_DATA: {
            while (ctx_.seg_index < ctx_.seg_count && processed < size) {
                uint8_t seg_len = ctx_.seg_table[ctx_.seg_index];

                // Periksa apakah sebagian data segmen sudah terbaca sebelumnya
                if (ctx_.seg_remaining > 0) {
                    seg_len = ctx_.seg_remaining;
                } else {
                    ctx_.seg_remaining = seg_len;
                }

                // Pastikan penyangga paket masih cukup
                if (ctx_.packet_len + seg_len > sizeof(ctx_.packet_buf)) {
                    ESP_LOGE(TAG, "Buffer paket meluap: %zu + %u > %zu", ctx_.packet_len, seg_len, sizeof(ctx_.packet_buf));
                    state_ = ParseState::FIND_PAGE;
                    ctx_.packet_len = 0;
                    ctx_.packet_continued = false;
                    ctx_.seg_remaining = 0;
                    ctx_.bytes_needed = 4;
                    return processed;
                }

                // Salin data segmen ke penyangga paket
                size_t to_copy = std::min(size - processed, (size_t)seg_len);
                memcpy(ctx_.packet_buf + ctx_.packet_len, data + processed, to_copy);

                processed += to_copy;
                ctx_.packet_len += to_copy;
                ctx_.body_offset += to_copy;
                ctx_.seg_remaining -= to_copy;

                // Periksa apakah segmen sudah lengkap
                if (ctx_.seg_remaining > 0) {
                    // Segmen belum lengkap, tunggu data berikutnya
                    return processed;
                }

                // Segmen sudah lengkap
                bool seg_continued = (ctx_.seg_table[ctx_.seg_index] == 255);

                if (!seg_continued) {
                    // Paket berakhir pada segmen ini
                    if (ctx_.packet_len) {
                        if (!opus_info_.head_seen) {
                            if (ctx_.packet_len >= 8 && memcmp(ctx_.packet_buf, "OpusHead", 8) == 0) {
                                opus_info_.head_seen = true;
                                if (ctx_.packet_len >= 19) {
                                    opus_info_.sample_rate = ctx_.packet_buf[12] |
                                        (ctx_.packet_buf[13] << 8) |
                                        (ctx_.packet_buf[14] << 16) |
                                        (ctx_.packet_buf[15] << 24);
                                    ESP_LOGD(TAG, "OpusHead found, sample_rate=%d", opus_info_.sample_rate);
                                }
                                ctx_.packet_len = 0;
                                ctx_.packet_continued = false;
                                ctx_.seg_index++;
                                ctx_.seg_remaining = 0;
                                continue;
                            }
                        }
                        if (!opus_info_.tags_seen) {
                            if (ctx_.packet_len >= 8 && memcmp(ctx_.packet_buf, "OpusTags", 8) == 0) {
                                opus_info_.tags_seen = true;
                                ESP_LOGD(TAG, "OpusTags found.");
                                ctx_.packet_len = 0;
                                ctx_.packet_continued = false;
                                ctx_.seg_index++;
                                ctx_.seg_remaining = 0;
                                continue;
                            }
                        }
                        if (opus_info_.head_seen && opus_info_.tags_seen) {
                            if (on_demuxer_finished_) {
                                on_demuxer_finished_(ctx_.packet_buf, opus_info_.sample_rate, ctx_.packet_len);
                            }
                        } else {
                            ESP_LOGW(TAG, "Kontainer Ogg saat ini belum memuat OpusHead atau OpusTags, paket dibuang");
                        }
                    }
                    ctx_.packet_len = 0;
                    ctx_.packet_continued = false;
                } else {
                    ctx_.packet_continued = true;
                }

                ctx_.seg_index++;
                ctx_.seg_remaining = 0;
            }

            if (ctx_.seg_index == ctx_.seg_count) {
                // Periksa apakah seluruh badan data halaman sudah selesai dibaca
                if (ctx_.body_offset < ctx_.body_size) {
                    ESP_LOGW(TAG, "Badan data belum lengkap: %zu/%zu", ctx_.body_offset, ctx_.body_size);
                }

                // Jika paket melintasi halaman, pertahankan penyangga paketnya
                if (!ctx_.packet_continued) {
                    ctx_.packet_len = 0;
                }

                // Lanjutkan pencarian ke halaman berikutnya
                state_ = ParseState::FIND_PAGE;
                ctx_.bytes_needed = 4;
                ctx_.data_offset = 0;
            }
            break;
        }
        }
    }

    return processed;
}
