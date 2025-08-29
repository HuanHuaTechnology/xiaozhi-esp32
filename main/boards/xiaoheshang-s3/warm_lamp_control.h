#ifndef __WARM_LAMP_CONTROL_H__
#define __WARM_LAMP_CONTROL_H__

#include "backlight.h"
#include "mcp_server.h"

class WarmLampControl {
private:
    gpio_num_t power_en_gpio_ = GPIO_NUM_NC;
    PwmBacklight dimmer_;
    bool power_on_ = false;
    uint8_t last_brightness_ = 100; // 记忆上次亮度

    void ApplyPower(bool on);

public:
    // lamp_gpio: 暖灯控制引脚（PWM），power_en_gpio：电源使能（可选）
    WarmLampControl(gpio_num_t lamp_gpio, gpio_num_t power_en_gpio = GPIO_NUM_NC, bool output_invert = false);
};

#endif // __WARM_LAMP_CONTROL_H__


