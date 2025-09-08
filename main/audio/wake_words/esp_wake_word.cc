#include "esp_wake_word.h"
#include <esp_log.h>


#define TAG "EspWakeWord"

EspWakeWord::EspWakeWord() {
}

EspWakeWord::~EspWakeWord() {
    if (wakenet_data_ != nullptr) {
        wakenet_iface_->destroy(wakenet_data_);
        esp_srmodel_deinit(wakenet_model_);
    }
}

bool EspWakeWord::Initialize(AudioCodec* codec) {
    codec_ = codec;

    wakenet_model_ = esp_srmodel_init("model");
    if (wakenet_model_ == nullptr || wakenet_model_->num == -1) {
        ESP_LOGE(TAG, "Failed to initialize wakenet model");
        return false;
    }
    if (wakenet_model_->num == 0) {
        ESP_LOGE(TAG, "No model found");
        return false;
    }
    // Prefer a model that matches "你好乐鑫" (Hi LeXin). We try to find names containing "hilexin/lexin".
    int chosen = 0;
    for (int i = 0; i < wakenet_model_->num; ++i) {
        char* name = wakenet_model_->model_name[i];
        if (name == nullptr) continue;
        // case-insensitive substring match
        std::string n(name);
        for (auto& c : n) c = (char)tolower((unsigned char)c);
        if (n.find("hilexin") != std::string::npos || n.find("hi_lexin") != std::string::npos || n.find("lexin") != std::string::npos) {
            chosen = i;
            break;
        }
    }
    if (wakenet_model_->num > 1) {
        ESP_LOGI(TAG, "Multiple models found. chosen=%d name=%s", chosen, wakenet_model_->model_name[chosen]);
    }
    char *model_name = wakenet_model_->model_name[chosen];
    wakenet_iface_ = (esp_wn_iface_t*)esp_wn_handle_from_name(model_name);
    wakenet_data_ = wakenet_iface_->create(model_name, DET_MODE_95);

    int frequency = wakenet_iface_->get_samp_rate(wakenet_data_);
    int audio_chunksize = wakenet_iface_->get_samp_chunksize(wakenet_data_);
    ESP_LOGI(TAG, "Wake word(%s),freq: %d, chunksize: %d", model_name, frequency, audio_chunksize);

    return true;
}

void EspWakeWord::OnWakeWordDetected(std::function<void(const std::string& wake_word)> callback) {
    wake_word_detected_callback_ = callback;
}

void EspWakeWord::Start() {
    running_ = true;
}

void EspWakeWord::Stop() {
    running_ = false;
}

void EspWakeWord::Feed(const std::vector<int16_t>& data) {
    if (wakenet_data_ == nullptr || !running_) {
        return;
    }

    int res = wakenet_iface_->detect(wakenet_data_, (int16_t *)data.data());
    if (res > 0) {
        last_detected_wake_word_ = wakenet_iface_->get_word_name(wakenet_data_, res);
        running_ = false;

        if (wake_word_detected_callback_) {
            wake_word_detected_callback_(last_detected_wake_word_);
        }
    }
}

size_t EspWakeWord::GetFeedSize() {
    if (wakenet_data_ == nullptr) {
        return 0;
    }
    return wakenet_iface_->get_samp_chunksize(wakenet_data_);
}

void EspWakeWord::EncodeWakeWordData() {
}

bool EspWakeWord::GetWakeWordOpus(std::vector<uint8_t>& opus) {
    return false;
}
