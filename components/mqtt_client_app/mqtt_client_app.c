#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mqtt_client_app.h"
#include "app_config.h"
#include "sensors.h"
#include "fan_control.h"
#include "mqtt_client.h"
#include "esp_log.h"

static const char *TAG = "mqtt_app";

#define TOPIC_STATE   "fanctrl/state"
#define TOPIC_CMD_FAN "fanctrl/cmd/fan_speed"

static esp_mqtt_client_handle_t s_client;

static void mqtt_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected");
        esp_mqtt_client_subscribe(s_client, TOPIC_CMD_FAN, 0);
        break;
    case MQTT_EVENT_DATA:
        if (event->topic_len && strncmp(event->topic, TOPIC_CMD_FAN, event->topic_len) == 0) {
            char buf[8] = {0};
            int len = event->data_len < sizeof(buf) - 1 ? event->data_len : sizeof(buf) - 1;
            memcpy(buf, event->data, len);
            int pct = atoi(buf);
            if (pct >= 0 && pct <= 100) {
                fan_control_set_duty_pct((uint8_t)pct);
                ESP_LOGI(TAG, "Fan speed set to %d%% via MQTT", pct);
            }
        }
        break;
    default:
        break;
    }
}

esp_err_t mqtt_client_app_init(void)
{
    app_config_t *cfg = app_config_get();
    if (!cfg->mqtt_enabled || strlen(cfg->mqtt_uri) == 0) {
        ESP_LOGI(TAG, "MQTT disabled or no broker configured, skipping");
        return ESP_OK;
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = cfg->mqtt_uri,
        .credentials.username = cfg->mqtt_user,
        .credentials.authentication.password = cfg->mqtt_pass,
    };

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!s_client) {
        ESP_LOGE(TAG, "esp_mqtt_client_init failed");
        return ESP_FAIL;
    }
    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    return esp_mqtt_client_start(s_client);
}

esp_err_t mqtt_client_app_publish_state(void)
{
    if (!s_client) {
        return ESP_ERR_INVALID_STATE;
    }
    sensors_reading_t reading;
    esp_err_t err = sensors_read(&reading);
    if (err != ESP_OK) {
        return err;
    }

    char payload[160];
    int n = snprintf(payload, sizeof(payload),
                      "{\"ntc1_c\":%.1f,\"ntc2_c\":%.1f,\"fan_pct\":%u}",
                      reading.ntc1_temp_c, reading.ntc2_temp_c, fan_control_get_duty_pct());
    esp_mqtt_client_publish(s_client, TOPIC_STATE, payload, n, 0, false);
    return ESP_OK;
}
