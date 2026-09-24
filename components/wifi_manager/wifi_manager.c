#include <string.h>
#include <stdbool.h>
#include "wifi_manager.h"
#include "app_config.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "esp_log.h"

static const char *TAG = "wifi_manager";
static volatile bool s_sta_connected;

static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_sta_connected = false;
        ESP_LOGW(TAG, "STA disconnected, retrying...");
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        s_sta_connected = true;
        ESP_LOGI(TAG, "STA got IP");
    }
}

esp_err_t wifi_manager_init(void)
{
    app_config_t *cfg = app_config_get();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t ap_config = { 0 };
    strlcpy((char *)ap_config.ap.ssid, cfg->ap_ssid, sizeof(ap_config.ap.ssid));
    ap_config.ap.ssid_len = strlen(cfg->ap_ssid);
    ap_config.ap.channel = 1;
    ap_config.ap.max_connection = 4;
    ap_config.ap.authmode = strlen(cfg->ap_pass) >= 8 ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    strlcpy((char *)ap_config.ap.password, cfg->ap_pass, sizeof(ap_config.ap.password));

    bool have_sta_creds = strlen(cfg->sta_ssid) > 0;
    wifi_mode_t mode = have_sta_creds ? WIFI_MODE_APSTA : WIFI_MODE_AP;

    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));

    if (have_sta_creds) {
        wifi_config_t sta_config = { 0 };
        strlcpy((char *)sta_config.sta.ssid, cfg->sta_ssid, sizeof(sta_config.sta.ssid));
        strlcpy((char *)sta_config.sta.password, cfg->sta_pass, sizeof(sta_config.sta.password));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    }

    ESP_ERROR_CHECK(esp_wifi_start());

    /* Default IDF SoftAP netif config already uses 192.168.4.1/24. */
    ESP_LOGI(TAG, "SoftAP '%s' up at 192.168.4.1, STA %s",
             cfg->ap_ssid, have_sta_creds ? "connecting..." : "not configured");
    return ESP_OK;
}

bool wifi_manager_is_sta_connected(void)
{
    return s_sta_connected;
}
