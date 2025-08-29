#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char* TAG = "TEST_GPIO0";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting GPIO0 test...");
    
    // 强制配置GPIO0为输入模式并启用内部上拉
    gpio_reset_pin(GPIO_NUM_0);
    gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_0, GPIO_PULLUP_ONLY);
    
    ESP_LOGI(TAG, "GPIO0 configured with internal pullup");
    ESP_LOGI(TAG, "GPIO0 level: %d", gpio_get_level(GPIO_NUM_0));
    
    while(1) {
        ESP_LOGI(TAG, "Device running normally! GPIO0 = %d", gpio_get_level(GPIO_NUM_0));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
