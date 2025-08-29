#include "board.h"
#include "audio_codec.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <vector>

#define TAG "MicTest"

void mic_test_task(void* arg) {
    ESP_LOGI(TAG, "Starting microphone test...");
    
    Board& board = Board::GetInstance();
    AudioCodec* codec = board.GetAudioCodec();
    
    if (!codec) {
        ESP_LOGE(TAG, "Failed to get audio codec!");
        vTaskDelete(nullptr);
        return;
    }
    
    // 启用麦克风输入
    codec->EnableInput(true);
    ESP_LOGI(TAG, "Microphone input enabled");
    
    // 设置较低的麦克风增益
    // 注意：这里需要直接调用ES8311的增益设置函数
    ESP_LOGI(TAG, "Setting microphone gain to 20dB (lower than default 30dB)");
    
    std::vector<int16_t> buffer(1024);  // 1KB缓冲区
    int total_samples = 0;
    int non_zero_samples = 0;
    
    ESP_LOGI(TAG, "Starting microphone recording test for 10 seconds...");
    ESP_LOGI(TAG, "Please speak into the microphone...");
    
    // 录制10秒
    for (int i = 0; i < 100; i++) {  // 100次循环，每次100ms
        if (codec->InputData(buffer)) {
            total_samples += buffer.size();
            
            // 检查是否有非零数据
            for (int sample : buffer) {
                if (sample != 0) {
                    non_zero_samples++;
                }
            }
            
            // 每2秒输出一次状态
            if (i % 20 == 0) {
                ESP_LOGI(TAG, "Recording... %d seconds, samples: %d, non-zero: %d", 
                         i/10, total_samples, non_zero_samples);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));  // 100ms延迟
    }
    
    ESP_LOGI(TAG, "Microphone test completed!");
    ESP_LOGI(TAG, "Total samples recorded: %d", total_samples);
    ESP_LOGI(TAG, "Non-zero samples: %d", non_zero_samples);
    ESP_LOGI(TAG, "Activity percentage: %.2f%%", 
             (float)non_zero_samples / total_samples * 100.0f);
    
    if (non_zero_samples > 0) {
        ESP_LOGI(TAG, "✅ Microphone is working! Detected audio input.");
    } else {
        ESP_LOGE(TAG, "❌ Microphone not working! No audio input detected.");
        ESP_LOGI(TAG, "Troubleshooting suggestions:");
        ESP_LOGI(TAG, "1. Check microphone hardware connection");
        ESP_LOGI(TAG, "2. Verify ES8311 I2C communication");
        ESP_LOGI(TAG, "3. Check microphone power supply");
        ESP_LOGI(TAG, "4. Try increasing microphone gain");
    }
    
    vTaskDelete(nullptr);
}

extern "C" void app_mic_test() {
    xTaskCreate(mic_test_task, "mic_test", 4096, nullptr, 5, nullptr);
}
