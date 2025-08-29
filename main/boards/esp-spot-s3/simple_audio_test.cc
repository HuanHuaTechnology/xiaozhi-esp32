#include "board.h"
#include "audio_codec.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2c.h>

#define TAG "SimpleAudioTest"

extern "C" void simple_audio_test() {
    ESP_LOGI(TAG, "=== Simple Audio Test Start ===");
    
    // 1. 测试I2C通信
    ESP_LOGI(TAG, "1. Testing I2C communication...");
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (0x18 << 1) | I2C_MASTER_WRITE, true);  // ES8311地址
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✅ I2C communication OK - ES8311 detected");
    } else {
        ESP_LOGE(TAG, "❌ I2C communication failed - ES8311 not found");
        ESP_LOGI(TAG, "Error: %s", esp_err_to_name(ret));
        return;
    }
    
    // 2. 获取音频编解码器
    ESP_LOGI(TAG, "2. Getting audio codec...");
    Board& board = Board::GetInstance();
    
    AudioCodec* codec = board.GetAudioCodec();
    if (!codec) {
        ESP_LOGE(TAG, "❌ Failed to get audio codec");
        return;
    }
    
    ESP_LOGI(TAG, "✅ Audio codec obtained successfully");
    ESP_LOGI(TAG, "   - Input sample rate: %d Hz", codec->input_sample_rate());
    ESP_LOGI(TAG, "   - Output sample rate: %d Hz", codec->output_sample_rate());
    ESP_LOGI(TAG, "   - Duplex mode: %s", codec->duplex() ? "Yes" : "No");
    
    // 3. 启用麦克风输入
    ESP_LOGI(TAG, "3. Enabling microphone input...");
    codec->EnableInput(true);
    ESP_LOGI(TAG, "✅ Microphone input enabled");
    
    // 4. 测试音频读取
    ESP_LOGI(TAG, "4. Testing audio input for 5 seconds...");
    std::vector<int16_t> buffer(512);
    int total_samples = 0;
    int non_zero_samples = 0;
    int max_amplitude = 0;
    
    for (int i = 0; i < 50; i++) {  // 5秒测试
        if (codec->InputData(buffer)) {
            total_samples += buffer.size();
            
            for (int sample : buffer) {
                if (sample != 0) {
                    non_zero_samples++;
                    if (abs(sample) > max_amplitude) {
                        max_amplitude = abs(sample);
                    }
                }
            }
            
            if (i % 10 == 0) {  // 每秒输出一次
                ESP_LOGI(TAG, "   Recording... %d seconds, samples: %d, non-zero: %d, max_amp: %d", 
                         i/10, total_samples, non_zero_samples, max_amplitude);
            }
        } else {
            ESP_LOGW(TAG, "   No audio data received at iteration %d", i);
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // 5. 输出测试结果
    ESP_LOGI(TAG, "5. Test Results:");
    ESP_LOGI(TAG, "   - Total samples: %d", total_samples);
    ESP_LOGI(TAG, "   - Non-zero samples: %d", non_zero_samples);
    ESP_LOGI(TAG, "   - Activity percentage: %.2f%%", 
             total_samples > 0 ? (float)non_zero_samples / total_samples * 100.0f : 0.0f);
    ESP_LOGI(TAG, "   - Maximum amplitude: %d", max_amplitude);
    
    if (non_zero_samples > 0) {
        ESP_LOGI(TAG, "✅ Microphone is working! Audio input detected.");
        if (max_amplitude > 100) {
            ESP_LOGI(TAG, "✅ Good signal strength detected.");
        } else {
            ESP_LOGW(TAG, "⚠️  Weak signal detected. Try speaking louder or check microphone gain.");
        }
    } else {
        ESP_LOGE(TAG, "❌ Microphone not working! No audio input detected.");
        ESP_LOGI(TAG, "Troubleshooting:");
        ESP_LOGI(TAG, "   1. Check microphone hardware connection");
        ESP_LOGI(TAG, "   2. Verify microphone power supply");
        ESP_LOGI(TAG, "   3. Check ES8311 configuration");
        ESP_LOGI(TAG, "   4. Try increasing microphone gain");
    }
    
    ESP_LOGI(TAG, "=== Simple Audio Test End ===");
}
