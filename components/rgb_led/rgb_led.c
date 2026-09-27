#include "rgb_led.h"
#include "board_config.h"
#include "led_strip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "rgb_led";

typedef struct {
    uint8_t r, g, b;
    uint8_t times;
} rgb_led_flash_req_t;

static led_strip_handle_t s_strip;
static uint8_t s_brightness = RGB_LED_DEFAULT_BRIGHTNESS;
static QueueHandle_t s_flash_queue;

static inline uint8_t scale(uint8_t channel)
{
    return (uint8_t)(((uint16_t)channel * s_brightness) / 255);
}

static esp_err_t apply(uint8_t r, uint8_t g, uint8_t b, bool on)
{
    if (!s_strip) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = on
        ? led_strip_set_pixel(s_strip, 0, scale(r), scale(g), scale(b))
        : led_strip_clear(s_strip);
    if (err != ESP_OK) {
        return err;
    }
    return on ? led_strip_refresh(s_strip) : ESP_OK;
}

static void rgb_led_task(void *arg)
{
    rgb_led_flash_req_t req;
    for (;;) {
        if (xQueueReceive(s_flash_queue, &req, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        for (uint8_t i = 0; i < req.times; i++) {
            apply(req.r, req.g, req.b, true);
            vTaskDelay(pdMS_TO_TICKS(500));
            apply(req.r, req.g, req.b, false);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

esp_err_t rgb_led_init(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = RGB_LED_GPIO,
        .max_leds = 1,
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, /* 10MHz */
        .flags.with_dma = false,
    };

    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "led_strip_new_rmt_device failed: %s", esp_err_to_name(err));
        return err;
    }

    err = led_strip_clear(s_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "led_strip_clear failed: %s", esp_err_to_name(err));
        return err;
    }

    s_flash_queue = xQueueCreate(4, sizeof(rgb_led_flash_req_t));
    if (!s_flash_queue) {
        ESP_LOGE(TAG, "Failed to create flash queue");
        return ESP_ERR_NO_MEM;
    }

    BaseType_t ok = xTaskCreate(rgb_led_task, "rgb_led", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "Failed to create RGB LED task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "RGB LED initialized on GPIO%d, brightness %u/255", RGB_LED_GPIO, s_brightness);
    return ESP_OK;
}

esp_err_t rgb_led_flash(uint8_t r, uint8_t g, uint8_t b, uint8_t times)
{
    if (!s_flash_queue) {
        return ESP_ERR_INVALID_STATE;
    }
    rgb_led_flash_req_t req = { .r = r, .g = g, .b = b, .times = times };
    return xQueueSend(s_flash_queue, &req, 0) == pdTRUE ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t rgb_led_set_brightness(uint8_t brightness)
{
    s_brightness = brightness;
    return ESP_OK;
}
