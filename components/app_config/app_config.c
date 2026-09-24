#include <string.h>
#include "app_config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "sdkconfig.h"
#include "esp_log.h"

static const char *TAG = "app_config";
static const char *NVS_NAMESPACE = "app_cfg";

static app_config_t s_cfg;

void app_config_reset_defaults(app_config_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    strlcpy(cfg->ap_ssid, CONFIG_FANCTRL_AP_SSID, sizeof(cfg->ap_ssid));
    strlcpy(cfg->ap_pass, CONFIG_FANCTRL_AP_PASSWORD, sizeof(cfg->ap_pass));
#ifdef CONFIG_FANCTRL_ENABLE_MQTT
    cfg->mqtt_enabled = true;
#endif
#ifdef CONFIG_FANCTRL_ENABLE_ZIGBEE
    cfg->zigbee_enabled = true;
#endif
#ifdef CONFIG_FANCTRL_ENABLE_THREAD
    cfg->thread_enabled = true;
#endif
#ifdef CONFIG_FANCTRL_ENABLE_ESPNOW
    cfg->espnow_enabled = true;
#endif
    cfg->fan_min_duty_pct = 0;
    cfg->fan_max_duty_pct = 100;
    cfg->target_temp_c = 45.0f;
}

static esp_err_t load_str(nvs_handle_t h, const char *key, char *out, size_t out_len)
{
    size_t len = out_len;
    esp_err_t err = nvs_get_str(h, key, out, &len);
    return err;
}

esp_err_t app_config_init(void)
{
    app_config_reset_defaults(&s_cfg);

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &h);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No stored config yet, using defaults");
        return ESP_OK;
    } else if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return err;
    }

    load_str(h, "sta_ssid", s_cfg.sta_ssid, sizeof(s_cfg.sta_ssid));
    load_str(h, "sta_pass", s_cfg.sta_pass, sizeof(s_cfg.sta_pass));
    load_str(h, "ap_ssid", s_cfg.ap_ssid, sizeof(s_cfg.ap_ssid));
    load_str(h, "ap_pass", s_cfg.ap_pass, sizeof(s_cfg.ap_pass));
    load_str(h, "mqtt_uri", s_cfg.mqtt_uri, sizeof(s_cfg.mqtt_uri));
    load_str(h, "mqtt_user", s_cfg.mqtt_user, sizeof(s_cfg.mqtt_user));
    load_str(h, "mqtt_pass", s_cfg.mqtt_pass, sizeof(s_cfg.mqtt_pass));
    load_str(h, "ota_url", s_cfg.ota_url, sizeof(s_cfg.ota_url));

    uint8_t u8;
    if (nvs_get_u8(h, "mqtt_en", &u8) == ESP_OK) s_cfg.mqtt_enabled = u8;
    if (nvs_get_u8(h, "zb_en", &u8) == ESP_OK) s_cfg.zigbee_enabled = u8;
    if (nvs_get_u8(h, "th_en", &u8) == ESP_OK) s_cfg.thread_enabled = u8;
    if (nvs_get_u8(h, "en_en", &u8) == ESP_OK) s_cfg.espnow_enabled = u8;
    if (nvs_get_u8(h, "fan_min", &u8) == ESP_OK) s_cfg.fan_min_duty_pct = u8;
    if (nvs_get_u8(h, "fan_max", &u8) == ESP_OK) s_cfg.fan_max_duty_pct = u8;

    int32_t target_temp_x10;
    if (nvs_get_i32(h, "tgt_temp_x10", &target_temp_x10) == ESP_OK) {
        s_cfg.target_temp_c = target_temp_x10 / 10.0f;
    }

    nvs_close(h);
    ESP_LOGI(TAG, "Loaded config from NVS");
    return ESP_OK;
}

app_config_t *app_config_get(void)
{
    return &s_cfg;
}

esp_err_t app_config_save(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return err;
    }

    nvs_set_str(h, "sta_ssid", s_cfg.sta_ssid);
    nvs_set_str(h, "sta_pass", s_cfg.sta_pass);
    nvs_set_str(h, "ap_ssid", s_cfg.ap_ssid);
    nvs_set_str(h, "ap_pass", s_cfg.ap_pass);
    nvs_set_str(h, "mqtt_uri", s_cfg.mqtt_uri);
    nvs_set_str(h, "mqtt_user", s_cfg.mqtt_user);
    nvs_set_str(h, "mqtt_pass", s_cfg.mqtt_pass);
    nvs_set_str(h, "ota_url", s_cfg.ota_url);
    nvs_set_u8(h, "mqtt_en", s_cfg.mqtt_enabled);
    nvs_set_u8(h, "zb_en", s_cfg.zigbee_enabled);
    nvs_set_u8(h, "th_en", s_cfg.thread_enabled);
    nvs_set_u8(h, "en_en", s_cfg.espnow_enabled);
    nvs_set_u8(h, "fan_min", s_cfg.fan_min_duty_pct);
    nvs_set_u8(h, "fan_max", s_cfg.fan_max_duty_pct);
    nvs_set_i32(h, "tgt_temp_x10", (int32_t)(s_cfg.target_temp_c * 10.0f));

    err = nvs_commit(h);
    nvs_close(h);
    return err;
}
