#include <stdio.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "app_config.h"
#include "sensors.h"
#include "fan_control.h"
#include "led_indicator.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "ota_manager.h"
#include "mqtt_client_app.h"
#include "espnow_manager.h"
#include "zigbee_manager.h"
#include "thread_manager.h"

static const char *TAG = "main";

/* Simple bang-bang control: fan runs at max above target, off with hysteresis below. */
#define FAN_HYSTERESIS_C 2.0f

static void control_task(void *arg)
{
    for (;;) {
        sensors_reading_t reading;
        if (sensors_read(&reading) == ESP_OK) {
            app_config_t *cfg = app_config_get();
            float temp = reading.ntc1_temp_c;
            uint8_t current = fan_control_get_duty_pct();

            if (temp >= cfg->target_temp_c) {
                fan_control_set_duty_pct(cfg->fan_max_duty_pct);
            } else if (temp <= cfg->target_temp_c - FAN_HYSTERESIS_C) {
                fan_control_set_duty_pct(0);
            }
            (void)current;

            mqtt_client_app_publish_state();
            espnow_manager_broadcast_state();
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(app_config_init());
    ESP_ERROR_CHECK(led_indicator_init());
    ESP_ERROR_CHECK(sensors_init());
    ESP_ERROR_CHECK(fan_control_init());
    ESP_ERROR_CHECK(wifi_manager_init());
    ESP_ERROR_CHECK(web_server_start());

    espnow_manager_init();
    mqtt_client_app_init();
    zigbee_manager_init();
    thread_manager_init();

    ota_manager_mark_valid();
    led_indicator_set_pattern(LED_PATTERN_DOUBLE_PULSE);

    xTaskCreate(control_task, "control_task", 4096, NULL, tskIDLE_PRIORITY + 2, NULL);
    ESP_LOGI(TAG, "Fan controller started");
}
