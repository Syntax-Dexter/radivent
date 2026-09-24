#include "led_indicator.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "led_indicator";
static volatile led_pattern_t s_pattern = LED_PATTERN_SLOW_BLINK;

static inline void led_write(int level)
{
    gpio_set_level(CONFIG_FANCTRL_LED_GPIO, level);
}

static void led_task(void *arg)
{
    for (;;) {
        switch (s_pattern) {
        case LED_PATTERN_OFF:
            led_write(0);
            vTaskDelay(pdMS_TO_TICKS(200));
            break;
        case LED_PATTERN_ON:
            led_write(1);
            vTaskDelay(pdMS_TO_TICKS(200));
            break;
        case LED_PATTERN_SLOW_BLINK:
            led_write(1);
            vTaskDelay(pdMS_TO_TICKS(500));
            led_write(0);
            vTaskDelay(pdMS_TO_TICKS(500));
            break;
        case LED_PATTERN_FAST_BLINK:
            led_write(1);
            vTaskDelay(pdMS_TO_TICKS(100));
            led_write(0);
            vTaskDelay(pdMS_TO_TICKS(100));
            break;
        case LED_PATTERN_DOUBLE_PULSE:
            led_write(1); vTaskDelay(pdMS_TO_TICKS(80));
            led_write(0); vTaskDelay(pdMS_TO_TICKS(120));
            led_write(1); vTaskDelay(pdMS_TO_TICKS(80));
            led_write(0); vTaskDelay(pdMS_TO_TICKS(720));
            break;
        }
    }
}

esp_err_t led_indicator_init(void)
{
    gpio_config_t io_cfg = {
        .pin_bit_mask = 1ULL << CONFIG_FANCTRL_LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&io_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %s", esp_err_to_name(err));
        return err;
    }

    BaseType_t ok = xTaskCreate(led_task, "led_indicator", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LED task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "LED indicator initialized on GPIO%d", CONFIG_FANCTRL_LED_GPIO);
    return ESP_OK;
}

void led_indicator_set_pattern(led_pattern_t pattern)
{
    s_pattern = pattern;
}
