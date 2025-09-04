#ifndef WARM_LED_CONTROL_H
#define WARM_LED_CONTROL_H

#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_timer.h>
#include <esp_err.h>

class WarmLedControl {
public:
    WarmLedControl(gpio_num_t power_pin, gpio_num_t control_pin);
    ~WarmLedControl();

    void On();
    void Off();
    void SetBrightness(int percent);  // 0-100
    void Blink(int interval_ms);      // toggle at interval

private:
    gpio_num_t power_pin_ = GPIO_NUM_NC;
    gpio_num_t control_pin_ = GPIO_NUM_NC;

    bool initialized_ = false;
    ledc_timer_t timer_num_ = LEDC_TIMER_1;
    ledc_channel_t channel_ = LEDC_CHANNEL_7;
    ledc_mode_t speed_mode_ = LEDC_LOW_SPEED_MODE;
    esp_timer_handle_t blink_timer_ = nullptr;
    bool blink_on_ = false;

    void InitializeIfNeeded();
    static void BlinkTimerCallback(void* arg);

    void RegisterTools();
};

#endif // WARM_LED_CONTROL_H


