#include <string.h>
#include "espnow_manager.h"
#include "app_config.h"
#include "sensors.h"
#include "fan_control.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_log.h"

static const char *TAG = "espnow_manager";
static const uint8_t s_broadcast_addr[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct __attribute__((packed)) {
    float   ntc1_temp_c;
    float   ntc2_temp_c;
    uint8_t fan_duty_pct;
} espnow_telemetry_t;

static void send_cb(const wifi_tx_info_t *info, esp_now_send_status_t status)
{
    if (status != ESP_NOW_SEND_SUCCESS) {
        ESP_LOGW(TAG, "ESP-NOW send failed");
    }
}

static void recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    ESP_LOGI(TAG, "ESP-NOW recv %d bytes", len);
}

esp_err_t espnow_manager_init(void)
{
    app_config_t *cfg = app_config_get();
    if (!cfg->espnow_enabled) {
        ESP_LOGI(TAG, "ESP-NOW disabled, skipping");
        return ESP_OK;
    }

    esp_err_t err = esp_now_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_now_init failed: %s", esp_err_to_name(err));
        return err;
    }
    esp_now_register_send_cb(send_cb);
    esp_now_register_recv_cb(recv_cb);

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, s_broadcast_addr, ESP_NOW_ETH_ALEN);
    peer.channel = 0;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;
    err = esp_now_add_peer(&peer);
    if (err != ESP_OK && err != ESP_ERR_ESPNOW_EXIST) {
        ESP_LOGE(TAG, "esp_now_add_peer failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "ESP-NOW initialized");
    return ESP_OK;
}

esp_err_t espnow_manager_broadcast_state(void)
{
    sensors_reading_t reading;
    esp_err_t err = sensors_read(&reading);
    if (err != ESP_OK) {
        return err;
    }
    espnow_telemetry_t msg = {
        .ntc1_temp_c = reading.ntc1_temp_c,
        .ntc2_temp_c = reading.ntc2_temp_c,
        .fan_duty_pct = fan_control_get_duty_pct(),
    };
    return esp_now_send(s_broadcast_addr, (const uint8_t *)&msg, sizeof(msg));
}
