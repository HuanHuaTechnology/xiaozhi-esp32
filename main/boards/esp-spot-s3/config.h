#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 16000

#define AUDIO_INPUT_REFERENCE    false

// ===== 音频I2S配置 - 根据您的实际原理图 =====
#define AUDIO_I2S_GPIO_MCLK      GPIO_NUM_4   // 您的板子：I2S主时钟
#define AUDIO_I2S_GPIO_WS        GPIO_NUM_17  // 您的板子：I2S字选 (S3_1017)
#define AUDIO_I2S_GPIO_BCLK      GPIO_NUM_16  // 您的板子：I2S位时钟 (S3_1016)
#define AUDIO_I2S_GPIO_DIN       GPIO_NUM_15  // 您的板子：I2S数据输入 (S3_1015)
#define AUDIO_I2S_GPIO_DOUT      GPIO_NUM_18  // 您的板子：I2S数据输出 (S3_1018)

// ===== 编解码器与功放配置 - 根据您的实际原理图 =====
#define AUDIO_CODEC_PA_PIN       GPIO_NUM_42  // 您的板子：功放控制引脚 (NS4150B CTRL引脚)
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_2   // 您的板子：I2C数据线 (S3_102)
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_1   // 您的板子：I2C时钟线 (S3_101)
#define AUDIO_CODEC_ES8311_ADDR  ES8311_CODEC_DEFAULT_ADDR

// ===== 按键配置 - 与ESP-Spot保持一致 =====
#define BOOT_BUTTON_GPIO         GPIO_NUM_0   // BOOT按钮
#define KEY_BUTTON_GPIO          GPIO_NUM_12  // 功能按键
#define LED_PIN                  GPIO_NUM_11  // LED指示灯

// ===== 电池电压采样 - 根据您的原理图 =====
#define VBAT_ADC_CHANNEL         ADC_CHANNEL_9  // 您的板子：GPIO10对应ADC_CH9

// ===== 电源控制 - 根据您的板子调整 =====
// 注意：您的板子没有独立的MCU和外设电源控制引脚
// 这些功能可能通过其他方式实现，暂时禁用
#define MCU_VCC_CTL              GPIO_NUM_NC   // 禁用：与I2S_MCLK冲突
#define PERP_VCC_CTL             GPIO_NUM_NC   // 禁用：与功放控制冲突

#define ADC_ATTEN                ADC_ATTEN_DB_12
#define ADC_WIDTH                ADC_BITWIDTH_DEFAULT
#define FULL_BATTERY_VOLTAGE     4100
#define EMPTY_BATTERY_VOLTAGE    3200

#endif // _BOARD_CONFIG_H_
