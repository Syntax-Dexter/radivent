#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_CONFIG_SSID_MAX_LEN     32
#define APP_CONFIG_PASS_MAX_LEN     64
#define APP_CONFIG_URI_MAX_LEN      128

/* Persisted device configuration, backed by NVS ("app_cfg" namespace). */
typedef struct {
    char     sta_ssid[APP_CONFIG_SSID_MAX_LEN];
    char     sta_pass[APP_CONFIG_PASS_MAX_LEN];
    char     ap_ssid[APP_CONFIG_SSID_MAX_LEN];
    char     ap_pass[APP_CONFIG_PASS_MAX_LEN];

    char     mqtt_uri[APP_CONFIG_URI_MAX_LEN];
    char     mqtt_user[APP_CONFIG_SSID_MAX_LEN];
    char     mqtt_pass[APP_CONFIG_PASS_MAX_LEN];
    bool     mqtt_enabled;

    char     ota_url[APP_CONFIG_URI_MAX_LEN];

    bool     zigbee_enabled;
    bool     thread_enabled;
    bool     espnow_enabled;

    uint8_t  fan_min_duty_pct;
    uint8_t  fan_max_duty_pct;
    float    target_temp_c;
} app_config_t;

/* Loads config from NVS, falling back to compile-time defaults for missing keys. */
esp_err_t app_config_init(void);

/* Returns a pointer to the in-RAM config (valid for the process lifetime). */
app_config_t *app_config_get(void);

/* Persists the current in-RAM config to NVS. */
esp_err_t app_config_save(void);

/* Resets in-RAM config to compile-time defaults (does not persist automatically). */
void app_config_reset_defaults(app_config_t *cfg);

#ifdef __cplusplus
}
#endif
