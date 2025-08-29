#ifndef LED_STRIP_CONTROL_H
#define LED_STRIP_CONTROL_H

#include "led/circular_strip.h"
#include "settings.h"
#include "mcp_server.h"
#include <esp_log.h>

class LedStripControl {
private:
    CircularStrip* led_strip_;
    int brightness_level_;  // 亮度等级 (0-8)

    inline int LevelToBrightness(int level) const {
        if (level < 0) level = 0;
        if (level > 8) level = 8;
        return (1 << level) - 1;  // 2^n - 1
    }

    inline StripColor RGBToColor(int red, int green, int blue) {
        if (red < 0) red = 0;
        if (red > 255) red = 255;
        if (green < 0) green = 0;
        if (green > 255) green = 255;
        if (blue < 0) blue = 0;
        if (blue > 255) blue = 255;
        return {static_cast<uint8_t>(red), static_cast<uint8_t>(green), static_cast<uint8_t>(blue)};
    }

public:
    explicit LedStripControl(CircularStrip* led_strip) : led_strip_(led_strip) {
        Settings settings("led_strip");
        brightness_level_ = settings.GetInt("brightness", 4);  // 默认等级4
        led_strip_->SetBrightness(LevelToBrightness(brightness_level_), 4);

        auto& mcp_server = McpServer::GetInstance();
        mcp_server.AddTool("self.led_strip.get_brightness",
            "Get the brightness of the led strip (0-8)",
            PropertyList(), [this](const PropertyList&) -> ReturnValue {
                return brightness_level_;
            });

        mcp_server.AddTool("self.led_strip.set_brightness",
            "Set the brightness of the led strip (0-8)",
            PropertyList({ Property("level", kPropertyTypeInteger, 0, 8) }),
            [this](const PropertyList& properties) -> ReturnValue {
                int level = properties["level"].value<int>();
                brightness_level_ = level;
                led_strip_->SetBrightness(LevelToBrightness(brightness_level_), 4);
                Settings settings("led_strip", true);
                settings.SetInt("brightness", brightness_level_);
                return true;
            });

        mcp_server.AddTool("self.led_strip.set_all_color",
            "Set the color of all leds.",
            PropertyList({
                Property("red", kPropertyTypeInteger, 0, 255),
                Property("green", kPropertyTypeInteger, 0, 255),
                Property("blue", kPropertyTypeInteger, 0, 255)
            }), [this](const PropertyList& properties) -> ReturnValue {
                int red = properties["red"].value<int>();
                int green = properties["green"].value<int>();
                int blue = properties["blue"].value<int>();
                led_strip_->SetAllColor(RGBToColor(red, green, blue));
                return true;
            });

        mcp_server.AddTool("self.led_strip.turn_off",
            "Turn off the led strip.",
            PropertyList(), [this](const PropertyList&) -> ReturnValue {
                led_strip_->SetBrightness(0, 4);
                return true;
            });
    }
}; 

#endif // LED_STRIP_CONTROL_H


