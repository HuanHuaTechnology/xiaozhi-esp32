#pragma once

// I2C Pins for ES8311 Codec (from schematic)
#define I2C_SCL_PIN 1
#define I2C_SDA_PIN 2

// I2S Pins for ES8311 Codec (from schematic)
#define I2S_BCLK_PIN   18
#define I2S_LRCK_PIN   17
#define I2S_DOUT_PIN   16 // To ES8311 SDIN
#define I2S_DIN_PIN    15 // From ES8311 SDOUT

// Audio Power Amplifier Control Pin (from schematic)
#define PA_CTL_PIN     14

// Programmable RGB LED Pin (from schematic)
#define NEOPIXEL_PIN   12

// Battery ADC Pin (from schematic)
#define VBAT_ADC_PIN   10 