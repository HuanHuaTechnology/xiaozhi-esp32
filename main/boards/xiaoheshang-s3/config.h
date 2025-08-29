#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// 采样率 - 使用ESP-Spot的配置
#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 16000

#define AUDIO_INPUT_REFERENCE    false

// I2S配置 - 根据您的原理图调整
#define AUDIO_I2S_GPIO_MCLK      GPIO_NUM_4   // 您的板子使用GPIO4
#define AUDIO_I2S_GPIO_WS        GPIO_NUM_17
#define AUDIO_I2S_GPIO_BCLK      GPIO_NUM_16
#define AUDIO_I2S_GPIO_DIN       GPIO_NUM_15
#define AUDIO_I2S_GPIO_DOUT      GPIO_NUM_7   // 您的板子使用GPIO7

// 编解码器与功放 - 根据您的原理图
#define AUDIO_CODEC_PA_PIN       GPIO_NUM_6   // 您的板子使用GPIO6
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_2
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_1
#define AUDIO_CODEC_ES8311_ADDR  ES8311_CODEC_DEFAULT_ADDR

// 按键配置 - 参考ESP-Spot
#define BOOT_BUTTON_GPIO         GPIO_NUM_0   // 修复：使用GPIO0作为BOOT按钮
#define KEY_BUTTON_GPIO          GPIO_NUM_12  // 添加：使用GPIO12作为功能按键
#define LED_PIN                  GPIO_NUM_11  // 简化：使用GPIO11作为LED

// 电池电压采样 - 根据您的原理图
#define VBAT_ADC_CHANNEL         ADC_CHANNEL_9  // S3: IO10
#define MCU_VCC_CTL              GPIO_NUM_4     // set 1 to power on MCU
#define PERP_VCC_CTL             GPIO_NUM_6     // set 1 to power on peripherals

#define ADC_ATTEN                ADC_ATTEN_DB_12
#define ADC_WIDTH                ADC_BITWIDTH_DEFAULT
#define FULL_BATTERY_VOLTAGE     4100
#define EMPTY_BATTERY_VOLTAGE    3200

// 暂时注释掉复杂的LED配置，先让基本功能工作
// #define LED_POWER_EN_GPIO        GPIO_NUM_12
// #define LED_WARM_CTL_GPIO        GPIO_NUM_13
// #define RGB_LED_DATA_GPIO        GPIO_NUM_11
// #define RGB_LED_NUM              8

#endif // _BOARD_CONFIG_H_


