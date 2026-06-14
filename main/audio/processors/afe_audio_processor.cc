#include "afe_audio_processor.h"
#include <esp_log.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

#define PROCESSOR_RUNNING 0x01

#define TAG "AfeAudioProcessor"

namespace {

constexpr float kTargetSpeechRms = 4500.0f;
constexpr float kQuietSpeechTargetRms = 3200.0f;
constexpr float kMaximumSpeechPostNsGain = 1.90f;
constexpr float kMaximumQuietPostNsGain = 1.45f;
constexpr float kPostNsGainSmoothing = 0.08f;
constexpr int kQuietSpeechRmsThreshold = 256;

float ComputeRms(const int16_t* data, size_t samples) {
    if (data == nullptr || samples == 0) {
        return 0.0f;
    }

    double sum = 0.0;
    for (size_t i = 0; i < samples; ++i) {
        double sample = static_cast<double>(data[i]);
        sum += sample * sample;
    }
    return static_cast<float>(std::sqrt(sum / static_cast<double>(samples)));
}

void ApplySmoothedGain(std::vector<int16_t>& data, float target_rms, float max_gain, const char* reason) {
    static float smoothed_gain = 1.0f;
    if (data.empty()) {
        return;
    }

    float rms = ComputeRms(data.data(), data.size());
    float requested_gain = 1.0f;
    if (rms > 1.0f && rms < target_rms) {
        requested_gain = std::min(max_gain, target_rms / rms);
    }

    smoothed_gain = (smoothed_gain * (1.0f - kPostNsGainSmoothing)) +
                    (requested_gain * kPostNsGainSmoothing);
    smoothed_gain = std::max(1.0f, std::min(max_gain, smoothed_gain));

    if (smoothed_gain <= 1.02f) {
        return;
    }

    for (auto& sample : data) {
        int32_t boosted = static_cast<int32_t>(sample * smoothed_gain);
        boosted = std::max<int32_t>(INT16_MIN, std::min<int32_t>(INT16_MAX, boosted));
        sample = static_cast<int16_t>(boosted);
    }

#ifdef CONFIG_AUDIO_DIAGNOSTIC
    static uint32_t log_counter = 0;
    if ((++log_counter % 50) == 0) {
        ESP_LOGI(TAG, "Post-NS gain %.2fx (%s), rms=%.0f", smoothed_gain, reason, rms);
    }
#endif
}

}  // namespace

AfeAudioProcessor::AfeAudioProcessor()
    : afe_data_(nullptr) {
    event_group_ = xEventGroupCreate();
}

void AfeAudioProcessor::Initialize(AudioCodec* codec, int frame_duration_ms, srmodel_list_t* models_list) {
    codec_ = codec;
    frame_samples_ = frame_duration_ms * 16000 / 1000;

    // Siapkan kapasitas penyangga keluaran dari awal.
    output_buffer_.reserve(frame_samples_);

    int ref_num = codec_->input_reference() ? 1 : 0;

    std::string input_format;
    for (int i = 0; i < codec_->input_channels() - ref_num; i++) {
        input_format.push_back('M');
    }
    for (int i = 0; i < ref_num; i++) {
        input_format.push_back('R');
    }

    srmodel_list_t *models;
    if (models_list == nullptr) {
        models = esp_srmodel_init("model");
    } else {
        models = models_list;
    }
    if (models == nullptr) {
        ESP_LOGE(TAG, "Daftar model ESP-SR tidak tersedia");
        return;
    }

    char* ns_model_name = esp_srmodel_filter(models, ESP_NSNET_PREFIX, NULL);
    char* vad_model_name = esp_srmodel_filter(models, ESP_VADN_PREFIX, NULL);
    
    afe_config_t* afe_config = afe_config_init(input_format.c_str(), NULL, AFE_TYPE_VC, AFE_MODE_HIGH_PERF);
    if (afe_config == nullptr) {
        ESP_LOGE(TAG, "Gagal membuat konfigurasi AFE");
        return;
    }
    afe_config->aec_mode = AEC_MODE_VOIP_HIGH_PERF;
    afe_config->vad_mode = VAD_MODE_0;
    afe_config->vad_min_noise_ms = 100;
    if (vad_model_name != nullptr) {
        afe_config->vad_model_name = vad_model_name;
    }

    if (ns_model_name != nullptr) {
        afe_config->ns_init = true;
        afe_config->ns_model_name = ns_model_name;
        afe_config->afe_ns_mode = AFE_NS_MODE_NET;
        ESP_LOGI(TAG, "Peredam bising aktif dengan model %s", ns_model_name);
    } else {
        afe_config->ns_init = false;
        ESP_LOGW(TAG, "Model peredam bising tidak tersedia, AFE berjalan tanpa NS");
    }

    // Hindari penguatan ganda karena level INMP441 sudah mendekati batas PCM.
    afe_config->agc_init = false;
    afe_config->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;

#ifdef CONFIG_USE_DEVICE_AEC
    afe_config->aec_init = true;
    afe_config->vad_init = false;
#else
    afe_config->aec_init = false;
    afe_config->vad_init = true;
#endif

    afe_iface_ = esp_afe_handle_from_config(afe_config);
    if (afe_iface_ == nullptr) {
        ESP_LOGE(TAG, "Antarmuka AFE tidak tersedia untuk konfigurasi saat ini");
        afe_config_free(afe_config);
        return;
    }

    afe_data_ = afe_iface_->create_from_config(afe_config);
    afe_config_free(afe_config);
    if (afe_data_ == nullptr) {
        ESP_LOGE(TAG, "Gagal membuat pemroses audio AFE");
        return;
    }
    
    BaseType_t task_result = xTaskCreate([](void* arg) {
        auto this_ = (AfeAudioProcessor*)arg;
        this_->AudioProcessorTask();
        vTaskDelete(NULL);
    }, "audio_communication", 4096, this, 3, NULL);
    if (task_result != pdPASS) {
        ESP_LOGE(TAG, "Gagal membuat tugas pemrosesan audio");
        afe_iface_->destroy(afe_data_);
        afe_data_ = nullptr;
    }
}

AfeAudioProcessor::~AfeAudioProcessor() {
    if (afe_data_ != nullptr) {
        afe_iface_->destroy(afe_data_);
    }
    vEventGroupDelete(event_group_);
}

size_t AfeAudioProcessor::GetFeedSize() {
    if (afe_data_ == nullptr) {
        return 0;
    }
    return afe_iface_->get_feed_chunksize(afe_data_);
}

void AfeAudioProcessor::Feed(std::vector<int16_t>&& data) {
    if (afe_data_ == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(input_buffer_mutex_);
    // Periksa status berjalan di dalam kunci agar tidak balapan dengan Stop().
    if (!IsRunning()) {
        return;
    }
    input_buffer_.insert(input_buffer_.end(), data.begin(), data.end());
    size_t chunk_size = afe_iface_->get_feed_chunksize(afe_data_) * codec_->input_channels();
    while (input_buffer_.size() >= chunk_size) {
        afe_iface_->feed(afe_data_, input_buffer_.data());
        input_buffer_.erase(input_buffer_.begin(), input_buffer_.begin() + chunk_size);
    }
}

void AfeAudioProcessor::Start() {
    is_speaking_ = false;
    xEventGroupSetBits(event_group_, PROCESSOR_RUNNING);
}

void AfeAudioProcessor::Stop() {
    xEventGroupClearBits(event_group_, PROCESSOR_RUNNING);
    is_speaking_ = false;

    std::lock_guard<std::mutex> lock(input_buffer_mutex_);
    if (afe_data_ != nullptr) {
        afe_iface_->reset_buffer(afe_data_);
    }
    input_buffer_.clear();
    output_buffer_.clear();
}

bool AfeAudioProcessor::IsRunning() {
    return xEventGroupGetBits(event_group_) & PROCESSOR_RUNNING;
}

void AfeAudioProcessor::OnOutput(std::function<void(std::vector<int16_t>&& data)> callback) {
    output_callback_ = callback;
}

void AfeAudioProcessor::OnVadStateChange(std::function<void(bool speaking)> callback) {
    vad_state_change_callback_ = callback;
}

void AfeAudioProcessor::AudioProcessorTask() {
    auto fetch_size = afe_iface_->get_fetch_chunksize(afe_data_);
    auto feed_size = afe_iface_->get_feed_chunksize(afe_data_);
    ESP_LOGI(TAG, "Audio communication task started, feed size: %d fetch size: %d",
        feed_size, fetch_size);

    while (true) {
        xEventGroupWaitBits(event_group_, PROCESSOR_RUNNING, pdFALSE, pdTRUE, portMAX_DELAY);

        auto res = afe_iface_->fetch_with_delay(afe_data_, portMAX_DELAY);
        if ((xEventGroupGetBits(event_group_) & PROCESSOR_RUNNING) == 0) {
            continue;
        }
        if (res == nullptr || res->ret_value == ESP_FAIL) {
            if (res != nullptr) {
                ESP_LOGI(TAG, "Kode error: %d", res->ret_value);
            }
            continue;
        }

        // Perubahan status VAD.
        if (vad_state_change_callback_) {
            if (res->vad_state == VAD_SPEECH && !is_speaking_) {
                is_speaking_ = true;
                vad_state_change_callback_(true);
            } else if (res->vad_state == VAD_SILENCE && is_speaking_) {
                is_speaking_ = false;
                vad_state_change_callback_(false);
            }
        }

        if (output_callback_) {
            size_t samples = res->data_size / sizeof(int16_t);
            
            // Tambahkan data ke penyangga.
            output_buffer_.insert(output_buffer_.end(), res->data, res->data + samples);
            
            // Keluarkan bingkai penuh saat penyangga sudah cukup.
            while (output_buffer_.size() >= frame_samples_) {
                if (output_buffer_.size() == frame_samples_) {
                    // Jika ukuran penyangga sama dengan satu bingkai, pindahkan seluruh penyangga.
                    if (res->vad_state == VAD_SPEECH) {
                        ApplySmoothedGain(output_buffer_, kTargetSpeechRms, kMaximumSpeechPostNsGain, "speech");
                    } else if (res->vad_state == VAD_SILENCE &&
                               ComputeRms(output_buffer_.data(), output_buffer_.size()) >= kQuietSpeechRmsThreshold) {
                        ApplySmoothedGain(output_buffer_, kQuietSpeechTargetRms, kMaximumQuietPostNsGain, "quiet");
                    }
                    output_callback_(std::move(output_buffer_));
                    output_buffer_.clear();
                    output_buffer_.reserve(frame_samples_);
                } else {
                    // Jika penyangga melebihi satu bingkai, salin satu bingkai lalu hapus dari penyangga.
                    std::vector<int16_t> frame(output_buffer_.begin(), output_buffer_.begin() + frame_samples_);
                    if (res->vad_state == VAD_SPEECH) {
                        ApplySmoothedGain(frame, kTargetSpeechRms, kMaximumSpeechPostNsGain, "speech");
                    } else if (res->vad_state == VAD_SILENCE &&
                               ComputeRms(frame.data(), frame.size()) >= kQuietSpeechRmsThreshold) {
                        ApplySmoothedGain(frame, kQuietSpeechTargetRms, kMaximumQuietPostNsGain, "quiet");
                    }
                    output_callback_(std::move(frame));
                    output_buffer_.erase(output_buffer_.begin(), output_buffer_.begin() + frame_samples_);
                }
            }
        }
    }
}

void AfeAudioProcessor::EnableDeviceAec(bool enable) {
    if (afe_iface_ == nullptr || afe_data_ == nullptr) {
        ESP_LOGW(TAG, "AFE belum siap untuk mengubah pengaturan AEC");
        return;
    }

    if (enable) {
#if CONFIG_USE_DEVICE_AEC
        afe_iface_->disable_vad(afe_data_);
        afe_iface_->enable_aec(afe_data_);
#else
        ESP_LOGE(TAG, "Device AEC tidak didukung");
#endif
    } else {
        afe_iface_->disable_aec(afe_data_);
        afe_iface_->enable_vad(afe_data_);
    }
}
