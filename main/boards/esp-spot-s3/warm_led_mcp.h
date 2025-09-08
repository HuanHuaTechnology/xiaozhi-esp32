#ifndef __ESP_SPOT_S3_WARM_LED_MCP_H__
#define __ESP_SPOT_S3_WARM_LED_MCP_H__

// Register MCP tools for controlling warm white LEDs (LED2-LED9 group).
// Tools:
// - self.warm_led.on
// - self.warm_led.off
// - self.warm_led.set_brightness { percent:int(0..100) }
// - self.warm_led.get_brightness
//
// Pins are taken from boards/esp-spot-s3/config.h:
//   WARM_LED_PWM_GPIO   (LEDC PWM output)
//   WARM_LED_POWER_GPIO (optional power switch, high = on)
// If pins are not defined or set to GPIO_NUM_NC, the tools will NOT be registered.

void RegisterWarmLedTools();

#endif // __ESP_SPOT_S3_WARM_LED_MCP_H__


