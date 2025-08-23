#include "sdkconfig.h"
#include "boards/common/wifi_board.h"
#include "audio/codecs/es8311_audio_codec.h"
#include "led/single_led.h"
#include "board.h"
#include "display.h"
#include <memory>

// Correct, modern IDF headers - use NEW I2C API
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define BOARD_TYPE_NAME "my-esp-spot"

// Voltage divider multiplier from schematic: (100k + 200k) / 200k = 1.5
#define POWER_ADC_BATT_VOLTAGE_MULTIPLIER 1.5

// ADC Channel for GPIO10 on ESP32-S3 is ADC1_CHANNEL_9
#define VBAT_ADC_CHANNEL ADC_CHANNEL_9

class MyEspSpotBoard : public WifiBoard {
private:
    std::unique_ptr<Led> led_;
    std::unique_ptr<AudioCodec> audio_codec_;
    i2c_master_bus_handle_t i2c_bus_;
    adc_oneshot_unit_handle_t adc1_handle_;
    adc_cali_handle_t adc1_cali_handle_ = nullptr;
    bool cali_enable_ = false;

public:
    MyEspSpotBoard() {
        // Init PA
        gpio_set_direction(static_cast<gpio_num_t>(PA_CTL_PIN), GPIO_MODE_OUTPUT);
        gpio_set_level(static_cast<gpio_num_t>(PA_CTL_PIN), 1);

        // Init LED
        led_ = std::make_unique<SingleLed>(static_cast<gpio_num_t>(NEOPIXEL_PIN));

        // Init ADC
        adc_oneshot_unit_init_cfg_t init_config1 = { .unit_id = ADC_UNIT_1 };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle_));
        adc_oneshot_chan_cfg_t config = {
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle_, VBAT_ADC_CHANNEL, &config));
        
        // ADC Calibration
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT_1,
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc1_cali_handle_);
        if (ret == ESP_OK) {
            cali_enable_ = true;
        }
    }

    // --- Correctly overriding functions from Board / WifiBoard ---

    AudioCodec* GetAudioCodec() override {
        if (!audio_codec_) {
            // Initialize I2C peripheral using NEW API
            i2c_master_bus_config_t i2c_bus_cfg = {
                .i2c_port = I2C_NUM_0,
                .sda_io_num = static_cast<gpio_num_t>(I2C_SDA_PIN),
                .scl_io_num = static_cast<gpio_num_t>(I2C_SCL_PIN),
                .clk_source = I2C_CLK_SRC_DEFAULT,
                .glitch_ignore_cnt = 7,
                .intr_priority = 0,
                .trans_queue_depth = 0,
                .flags = {
                    .enable_internal_pullup = 1,
                },
            };
            ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus_));

            audio_codec_ = std::make_unique<Es8311AudioCodec>(&i2c_bus_, I2C_NUM_0, 16000, 16000,
                static_cast<gpio_num_t>(I2S_BCLK_PIN), static_cast<gpio_num_t>(I2S_LRCK_PIN),
                static_cast<gpio_num_t>(I2S_DIN_PIN), static_cast<gpio_num_t>(I2S_DOUT_PIN),
                GPIO_NUM_NC, GPIO_NUM_NC, 0, false, false);
        }
        return audio_codec_.get();
    }

    Led* GetLed() override {
        return led_.get();
    }

    Display* GetDisplay() override {
        return nullptr;
    }
    
    std::string GetBoardType() override {
        return BOARD_TYPE_NAME;
    }

    bool GetBatteryLevel(int &level, bool& charging, bool& discharging) override {
        int adc_raw;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle_, VBAT_ADC_CHANNEL, &adc_raw));
        
        int voltage_mv = adc_raw;
        if (cali_enable_) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle_, adc_raw, &voltage_mv));
        }

        float voltage = static_cast<float>(voltage_mv) / 1000.0f * POWER_ADC_BATT_VOLTAGE_MULTIPLIER;
        
        // Simple mapping from voltage to percentage (3.0V = 0%, 4.2V = 100%)
        level = (int)((voltage - 3.0f) / (4.2f - 3.0f) * 100.0f);
        if (level < 0) level = 0;
        if (level > 100) level = 100;
        
        charging = false; // Add logic if charging status is available
        discharging = true;

        return true;
    }
};

DECLARE_BOARD(MyEspSpotBoard); 