#include "warm_led_mcp.h"
#include "config.h"
#include "mcp_server.h"
#include "device_state_event.h"

#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_log.h>

static const char* TAG_WARM = "WarmLED";

#ifndef WARM_LED_PWM_GPIO
#define WARM_LED_PWM_GPIO GPIO_NUM_NC
#endif
#ifndef WARM_LED_POWER_GPIO
#define WARM_LED_POWER_GPIO GPIO_NUM_NC
#endif

static bool g_inited = false;
static uint8_t g_brightness = 100; // percent
static bool g_breathe_enabled = false;
static int g_breathe_low = 5;      // percent
static int g_breathe_high = 80;    // percent
static int g_breathe_interval_ms = 40;
static bool g_breathe_increase = true;
static int g_breathe_current = 20; // percent
static esp_timer_handle_t g_breathe_timer = nullptr;
static bool g_force_off = false;    // honor explicit user OFF until turned on

static void ensure_init() {
    if (g_inited) return;
    if (WARM_LED_PWM_GPIO == GPIO_NUM_NC) {
        ESP_LOGW(TAG_WARM, "WARM_LED_PWM_GPIO not defined, skip tools");
        return;
    }

    if (WARM_LED_POWER_GPIO != GPIO_NUM_NC) {
        gpio_config_t pwr = {};
        int pin = (int)WARM_LED_POWER_GPIO;
        pwr.pin_bit_mask = (pin >= 0) ? (1ULL << pin) : 0ULL;
        pwr.mode = GPIO_MODE_OUTPUT;
        pwr.pull_up_en = GPIO_PULLUP_DISABLE;
        pwr.pull_down_en = GPIO_PULLDOWN_DISABLE;
        pwr.intr_type = GPIO_INTR_DISABLE;
        gpio_config(&pwr);
        gpio_set_level(WARM_LED_POWER_GPIO, 1);
    }

    ledc_timer_config_t timer = {};
    timer.duty_resolution = LEDC_TIMER_13_BIT;
    timer.freq_hz = 4000;
    timer.speed_mode = LEDC_LOW_SPEED_MODE;
    timer.timer_num = LEDC_TIMER_2;
    timer.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t ch = {};
    ch.channel = LEDC_CHANNEL_2;
    ch.duty = 0;
    ch.gpio_num = WARM_LED_PWM_GPIO;
    ch.speed_mode = LEDC_LOW_SPEED_MODE;
    ch.hpoint = 0;
    ch.timer_sel = LEDC_TIMER_2;
    ch.flags.output_invert = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&ch));

    if (g_breathe_timer == nullptr) {
        esp_timer_create_args_t timer_args = {
            .callback = [](void* /*arg*/) {
                if (!g_breathe_enabled) return;
                // Update current breathe level
                if (g_breathe_increase) {
                    g_breathe_current++;
                    if (g_breathe_current >= g_breathe_high) {
                        g_breathe_current = g_breathe_high;
                        g_breathe_increase = false;
                    }
                } else {
                    g_breathe_current--;
                    if (g_breathe_current <= g_breathe_low) {
                        g_breathe_current = g_breathe_low;
                        g_breathe_increase = true;
                    }
                }
                if (g_force_off) return; // respect forced-off, do not light up
                g_brightness = (uint8_t)g_breathe_current;
                if (WARM_LED_POWER_GPIO != GPIO_NUM_NC && g_brightness > 0) {
                    gpio_set_level(WARM_LED_POWER_GPIO, 1);
                }
                // Apply duty
                uint32_t duty = (uint32_t)g_brightness * 8191 / 100;
                ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, duty);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
            },
            .arg = nullptr,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "warm_breathe"
        };
        ESP_ERROR_CHECK(esp_timer_create(&timer_args, &g_breathe_timer));
    }

    g_inited = true;
}

static void apply_brightness() {
    if (!g_inited) return;
    if (g_force_off) {
        // Ensure hardware stays off
        if (WARM_LED_POWER_GPIO != GPIO_NUM_NC) gpio_set_level(WARM_LED_POWER_GPIO, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
        return;
    }
    uint32_t duty = (uint32_t)g_brightness * 8191 / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
    if (WARM_LED_POWER_GPIO != GPIO_NUM_NC) gpio_set_level(WARM_LED_POWER_GPIO, g_brightness > 0 ? 1 : 0);
}

void RegisterWarmLedTools() {
    ensure_init();
    if (!g_inited) return;

    auto& mcp = McpServer::GetInstance();

    mcp.AddTool("self.warm_led.on", "打开暖白灯(LED2-LED9)", PropertyList(), [](const PropertyList&) -> ReturnValue {
        ensure_init();
        g_force_off = false;
        if (WARM_LED_POWER_GPIO != GPIO_NUM_NC) gpio_set_level(WARM_LED_POWER_GPIO, 1);
        g_brightness = g_brightness == 0 ? 80 : g_brightness; // default 80%
        apply_brightness();
        ESP_LOGI(TAG_WARM, "MCP:on brightness=%u", (unsigned)g_brightness);
        return true;
    });

    mcp.AddTool("self.warm_led.off", "关闭暖白灯(LED2-LED9)", PropertyList(), [](const PropertyList&) -> ReturnValue {
        ensure_init();
        g_force_off = true;
        g_breathe_enabled = false;
        if (g_breathe_timer) esp_timer_stop(g_breathe_timer);
        g_brightness = 0;
        if (WARM_LED_POWER_GPIO != GPIO_NUM_NC) gpio_set_level(WARM_LED_POWER_GPIO, 0);
        apply_brightness();
        ESP_LOGI(TAG_WARM, "MCP:off");
        return true;
    });

    mcp.AddTool("self.warm_led.breathe_on", "开启暖白灯呼吸效果", PropertyList({
        Property("low", kPropertyTypeInteger, 5, 0, 100),
        Property("high", kPropertyTypeInteger, 80, 0, 100),
        Property("interval_ms", kPropertyTypeInteger, 40, 10, 1000)
    }), [](const PropertyList& props) -> ReturnValue {
        ensure_init();
        int low = props["low"].value<int>();
        int high = props["high"].value<int>();
        int interval = props["interval_ms"].value<int>();
        if (low < 0) { low = 0; } else if (low > 100) { low = 100; }
        if (high < 0) { high = 0; } else if (high > 100) { high = 100; }
        if (low > high) { int t = low; low = high; high = t; }
        g_breathe_low = low;
        g_breathe_high = high;
        g_breathe_interval_ms = interval;
        g_breathe_current = g_breathe_low;
        g_breathe_increase = true;
        g_force_off = false; // enabling breathing cancels forced off
        g_breathe_enabled = true;
        if (WARM_LED_POWER_GPIO != GPIO_NUM_NC) gpio_set_level(WARM_LED_POWER_GPIO, 1);
        if (g_breathe_timer) esp_timer_start_periodic(g_breathe_timer, (uint64_t)g_breathe_interval_ms * 1000);
        ESP_LOGI(TAG_WARM, "MCP:breathe_on low=%d high=%d interval=%d", g_breathe_low, g_breathe_high, g_breathe_interval_ms);
        return true;
    });

    mcp.AddTool("self.warm_led.breathe_off", "关闭暖白灯呼吸效果", PropertyList(), [](const PropertyList&) -> ReturnValue {
        ensure_init();
        g_breathe_enabled = false;
        if (g_breathe_timer) esp_timer_stop(g_breathe_timer);
        ESP_LOGI(TAG_WARM, "MCP:breathe_off");
        return true;
    });

    mcp.AddTool("self.warm_led.set_brightness", "设置暖白灯亮度(0-100%)", PropertyList({
        Property("percent", kPropertyTypeInteger, 80, 0, 100)
    }), [](const PropertyList& props) -> ReturnValue {
        ensure_init();
        // Manual brightness change disables breathing
        g_breathe_enabled = false;
        if (g_breathe_timer) esp_timer_stop(g_breathe_timer);
        int p = props["percent"].value<int>();
        if (p < 0) p = 0;
        if (p > 100) p = 100;
        g_brightness = (uint8_t)p;
        g_force_off = (p == 0);
        if (WARM_LED_POWER_GPIO != GPIO_NUM_NC) gpio_set_level(WARM_LED_POWER_GPIO, p > 0 ? 1 : 0);
        apply_brightness();
        ESP_LOGI(TAG_WARM, "MCP:set_brightness=%u", (unsigned)g_brightness);
        return true;
    });

    mcp.AddTool("self.warm_led.get_brightness", "获取暖白灯亮度(%)", PropertyList(), [](const PropertyList&) -> ReturnValue {
        char buf[32];
        snprintf(buf, sizeof(buf), "{\"percent\":%u}", (unsigned)g_brightness);
        return std::string(buf);
    });

    // Follow device speaking/listening state
    DeviceStateEventManager::GetInstance().RegisterStateChangeCallback([](DeviceState /*prev*/, DeviceState cur){
        ensure_init();
        if (g_force_off) {
            // Keep LEDs off regardless of state changes
            g_breathe_enabled = false;
            if (g_breathe_timer) esp_timer_stop(g_breathe_timer);
            g_brightness = 0;
            apply_brightness();
            return;
        }
        switch (cur) {
            case kDeviceStateSpeaking:
                // Faster, brighter breathing while speaking
                g_breathe_low = 20;
                g_breathe_high = 100;
                g_breathe_interval_ms = 25;
                g_breathe_current = g_breathe_low;
                g_breathe_increase = true;
                g_breathe_enabled = true;
                if (WARM_LED_POWER_GPIO != GPIO_NUM_NC) gpio_set_level(WARM_LED_POWER_GPIO, 1);
                if (g_breathe_timer) esp_timer_start_periodic(g_breathe_timer, (uint64_t)g_breathe_interval_ms * 1000);
                break;
            case kDeviceStateListening:
            case kDeviceStateAudioTesting:
                // Slower, dimmer breathing while listening/testing
                g_breathe_low = 5;
                g_breathe_high = 40;
                g_breathe_interval_ms = 60;
                g_breathe_current = g_breathe_low;
                g_breathe_increase = true;
                g_breathe_enabled = true;
                if (WARM_LED_POWER_GPIO != GPIO_NUM_NC) gpio_set_level(WARM_LED_POWER_GPIO, 1);
                if (g_breathe_timer) esp_timer_start_periodic(g_breathe_timer, (uint64_t)g_breathe_interval_ms * 1000);
                break;
            case kDeviceStateIdle:
            case kDeviceStateUnknown:
            default:
                // Stop breathing; keep last brightness
                g_breathe_enabled = false;
                if (g_breathe_timer) esp_timer_stop(g_breathe_timer);
                break;
        }
    });
}


