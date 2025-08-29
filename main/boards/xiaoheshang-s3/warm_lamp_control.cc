#include "warm_lamp_control.h"
#include <esp_log.h>

static const char* TAG_WARM_LAMP = "WarmLamp";

void WarmLampControl::ApplyPower(bool on) {
    if (power_en_gpio_ != GPIO_NUM_NC) {
        gpio_config_t cfg = {
            .pin_bit_mask = (1ULL << power_en_gpio_),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&cfg));
        gpio_set_level(power_en_gpio_, on ? 1 : 0);
    }
}

WarmLampControl::WarmLampControl(gpio_num_t lamp_gpio, gpio_num_t power_en_gpio, bool output_invert)
    : power_en_gpio_(power_en_gpio), dimmer_(lamp_gpio, output_invert) {
    // 默认启动关闭但记忆亮度
    dimmer_.SetBrightness(0);

    auto& mcp = McpServer::GetInstance();

    // 获取状态
    mcp.AddTool("self.warm_lamp.get_state", "Get the power and brightness (0-100)", PropertyList(), [this](const PropertyList&) -> ReturnValue {
        return std::string("{\"power\":") + (power_on_ ? "true" : "false") + ",\"brightness\":" + std::to_string(dimmer_.brightness()) + "}";
    });

    // 开关
    mcp.AddTool("self.warm_lamp.turn_on", "Turn on the warm lamp", PropertyList(), [this](const PropertyList&) -> ReturnValue {
        ApplyPower(true);
        power_on_ = true;
        uint8_t target = last_brightness_ > 0 ? last_brightness_ : 100;
        dimmer_.SetBrightness(target);
        return true;
    });
    mcp.AddTool("self.warm_lamp.turn_off", "Turn off the warm lamp", PropertyList(), [this](const PropertyList&) -> ReturnValue {
        dimmer_.SetBrightness(0);
        ApplyPower(false);
        power_on_ = false;
        return true;
    });

    // 设定亮度 0-100
    mcp.AddTool("self.warm_lamp.set_brightness", "Set brightness 0-100", PropertyList({
        Property("value", kPropertyTypeInteger, 0, 100)
    }), [this](const PropertyList& props) -> ReturnValue {
        int value = props["value"].value<int>();
        if (!power_on_) {
            ApplyPower(true);
            power_on_ = true;
        }
        last_brightness_ = static_cast<uint8_t>(value);
        dimmer_.SetBrightness(last_brightness_);
        return true;
    });
}


