#include "warm_led_control.h"
#include "mcp_server.h"
#include <esp_log.h>

static const char* TAG = "WarmLed";

WarmLedControl::WarmLedControl(gpio_num_t power_pin, gpio_num_t control_pin)
    : power_pin_(power_pin), control_pin_(control_pin) {
    InitializeIfNeeded();
    RegisterTools();
}

WarmLedControl::~WarmLedControl() {
    if (blink_timer_) {
        esp_timer_stop(blink_timer_);
        esp_timer_delete(blink_timer_);
        blink_timer_ = nullptr;
    }
    if (initialized_) {
        ledc_stop(speed_mode_, channel_, 0);
    }
}

void WarmLedControl::InitializeIfNeeded() {
    if (initialized_) return;

    if (power_pin_ != GPIO_NUM_NC) {
        gpio_config_t io_power = { .pin_bit_mask = (1ULL << power_pin_), .mode = GPIO_MODE_OUTPUT,
                                   .pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                   .intr_type = GPIO_INTR_DISABLE };
        gpio_config(&io_power);
        gpio_set_level(power_pin_, 1);
    }

    // Use LEDC PWM on control pin for dimming
    ledc_timer_config_t timer = {};
    timer.speed_mode = speed_mode_;
    timer.duty_resolution = LEDC_TIMER_13_BIT;
    timer.timer_num = timer_num_;
    timer.freq_hz = 1000;
    timer.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t ch = {};
    ch.gpio_num = control_pin_;
    ch.speed_mode = speed_mode_;
    ch.channel = channel_;
    ch.intr_type = LEDC_INTR_DISABLE;
    ch.timer_sel = timer_num_;
    ch.duty = 0;
    ch.hpoint = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&ch));

    esp_timer_create_args_t targs = {};
    targs.callback = &WarmLedControl::BlinkTimerCallback;
    targs.arg = this;
    targs.dispatch_method = ESP_TIMER_TASK;
    targs.name = "warm_led_blink";
    ESP_ERROR_CHECK(esp_timer_create(&targs, &blink_timer_));

    initialized_ = true;
}

void WarmLedControl::On() {
    InitializeIfNeeded();
    esp_timer_stop(blink_timer_);
    ledc_set_duty(speed_mode_, channel_, (1 << 13) - 1);
    ledc_update_duty(speed_mode_, channel_);
}

void WarmLedControl::Off() {
    InitializeIfNeeded();
    esp_timer_stop(blink_timer_);
    ledc_set_duty(speed_mode_, channel_, 0);
    ledc_update_duty(speed_mode_, channel_);
}

void WarmLedControl::SetBrightness(int percent) {
    InitializeIfNeeded();
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    esp_timer_stop(blink_timer_);
    uint32_t duty = ((1 << 13) - 1) * percent / 100;
    ledc_set_duty(speed_mode_, channel_, duty);
    ledc_update_duty(speed_mode_, channel_);
}

void WarmLedControl::Blink(int interval_ms) {
    InitializeIfNeeded();
    esp_timer_stop(blink_timer_);
    blink_on_ = false;
    esp_timer_start_periodic(blink_timer_, interval_ms * 1000);
}

void WarmLedControl::BlinkTimerCallback(void* arg) {
    auto* self = static_cast<WarmLedControl*>(arg);
    self->blink_on_ = !self->blink_on_;
    uint32_t duty = self->blink_on_ ? ((1 << 13) - 1) : 0;
    ledc_set_duty(self->speed_mode_, self->channel_, duty);
    ledc_update_duty(self->speed_mode_, self->channel_);
}

void WarmLedControl::RegisterTools() {
    auto& mcp = McpServer::GetInstance();
    mcp.AddTool("self.warm_led.on", "Turn on warm LEDs", PropertyList(), [this](const PropertyList&) -> ReturnValue {
        On();
        return true;
    });
    mcp.AddTool("self.warm_led.off", "Turn off warm LEDs", PropertyList(), [this](const PropertyList&) -> ReturnValue {
        Off();
        return true;
    });
    mcp.AddTool("self.warm_led.set_brightness", "Set warm LED brightness (0-100)",
                PropertyList({ Property("percent", kPropertyTypeInteger, 0, 100) }),
                [this](const PropertyList& props) -> ReturnValue {
                    int p = props["percent"].value<int>();
                    SetBrightness(p);
                    return true;
                });
    mcp.AddTool("self.warm_led.blink", "Blink warm LEDs with interval (ms)",
                PropertyList({ Property("interval", kPropertyTypeInteger, 0, 2000) }),
                [this](const PropertyList& props) -> ReturnValue {
                    int interval = props["interval"].value<int>();
                    Blink(interval);
                    return true;
                });
}


