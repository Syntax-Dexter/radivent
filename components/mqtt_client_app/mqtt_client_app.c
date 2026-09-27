#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "mqtt_client_app.h"
#include "app_config.h"
#include "sensors.h"
#include "fan_control.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "wifi_manager.h"
#include "cJSON.h"

static const char *TAG = "mqtt_app";

#define TOPIC_STATE      "fanctrl/state"
#define TOPIC_AVAIL      "fanctrl/status"
#define TOPIC_CMD_FAN    "fanctrl/cmd/fan_speed"
#define HA_DISCOVERY_PREFIX "homeassistant"

static esp_mqtt_client_handle_t s_client;
static volatile bool s_connected;
static bool s_started;
static char s_device_id[24]; /* e.g. "fanctrl_a1b2c3d4e5f6" */

static void build_device_id(void)
{
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(s_device_id, sizeof(s_device_id), "fanctrl_%02x%02x%02x%02x%02x%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/* Publishes a retained Home Assistant MQTT Discovery config for one sensor entity. */
static void publish_discovery_sensor(const char *object_id, const char *name,
                                      const char *value_template, const char *unit,
                                      const char *device_class)
{
    cJSON *root = cJSON_CreateObject();
    char unique_id[48];
    char state_topic[64];
    char config_topic[96];

    snprintf(unique_id, sizeof(unique_id), "%s_%s", s_device_id, object_id);
    snprintf(state_topic, sizeof(state_topic), "%s", TOPIC_STATE);
    snprintf(config_topic, sizeof(config_topic), "%s/sensor/%s/%s/config",
             HA_DISCOVERY_PREFIX, s_device_id, object_id);

    cJSON_AddStringToObject(root, "name", name);
    cJSON_AddStringToObject(root, "unique_id", unique_id);
    cJSON_AddStringToObject(root, "state_topic", state_topic);
    cJSON_AddStringToObject(root, "value_template", value_template);
    cJSON_AddStringToObject(root, "availability_topic", TOPIC_AVAIL);
    cJSON_AddStringToObject(root, "payload_available", "online");
    cJSON_AddStringToObject(root, "payload_not_available", "offline");
    if (unit) {
        cJSON_AddStringToObject(root, "unit_of_measurement", unit);
    }
    if (device_class) {
        cJSON_AddStringToObject(root, "device_class", device_class);
    }

    cJSON *device = cJSON_AddObjectToObject(root, "device");
    cJSON *ids = cJSON_AddArrayToObject(device, "identifiers");
    cJSON_AddItemToArray(ids, cJSON_CreateString(s_device_id));
    cJSON_AddStringToObject(device, "name", "Fan Controller");
    cJSON_AddStringToObject(device, "manufacturer", "DIY");
    cJSON_AddStringToObject(device, "model", "ESP32-C6 Fan Controller");

    char *payload = cJSON_PrintUnformatted(root);
    if (payload) {
        esp_mqtt_client_publish(s_client, config_topic, payload, 0, 1, true);
        cJSON_free(payload);
    }
    cJSON_Delete(root);
}

static void publish_discovery_configs(void)
{
    publish_discovery_sensor("ntc1", "Fan Controller NTC1 Temperature",
                              "{{ value_json.ntc1_c }}", "\xC2\xB0" "C", "temperature");
    publish_discovery_sensor("ntc2", "Fan Controller NTC2 Temperature",
                              "{{ value_json.ntc2_c }}", "\xC2\xB0" "C", "temperature");
    publish_discovery_sensor("fan_rpm", "Fan Controller Fan Speed",
                              "{{ value_json.fan_rpm }}", "RPM", NULL);
    publish_discovery_sensor("wifi_rssi", "Fan Controller Wi-Fi RSSI",
                              "{{ value_json.wifi_rssi_dbm }}", "dBm", "signal_strength");
    publish_discovery_sensor("wifi_snr", "Fan Controller Wi-Fi SNR",
                              "{{ value_json.wifi_snr_db }}", "dB", NULL);
}

static void mqtt_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        s_connected = true;
        ESP_LOGI(TAG, "MQTT connected");
        esp_mqtt_client_subscribe(s_client, TOPIC_CMD_FAN, 0);
        publish_discovery_configs();
        esp_mqtt_client_publish(s_client, TOPIC_AVAIL, "online", 0, 1, true);
        break;
    case MQTT_EVENT_DISCONNECTED:
        s_connected = false;
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

/* Starts the MQTT client once, called either immediately (if Wi-Fi is already
 * up) or from the IP-got event handler below. */
static void start_mqtt_client(void)
{
    if (!s_client || s_started) {
        return;
    }
    s_started = true;
    esp_mqtt_client_start(s_client);
}

static void ip_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    if (base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        start_mqtt_client();
    }
}

esp_err_t mqtt_client_app_init(void)
{
    app_config_t *cfg = app_config_get();
    if (!cfg->mqtt_enabled || strlen(cfg->mqtt_uri) == 0) {
        ESP_LOGI(TAG, "MQTT disabled or no broker configured, skipping");
        return ESP_OK;
    }

    char normalized_uri[APP_CONFIG_URI_MAX_LEN];
    const char *mqtt_uri = cfg->mqtt_uri;
    if (!strstr(cfg->mqtt_uri, "://")) {
        /* Accept the convenient host:port form used by the web UI. */
        static const char scheme[] = "mqtt://";
        size_t uri_len = strlen(cfg->mqtt_uri);
        if (uri_len > sizeof(normalized_uri) - sizeof(scheme)) {
            ESP_LOGE(TAG, "MQTT URI is too long to normalize");
            return ESP_ERR_INVALID_SIZE;
        }
        memcpy(normalized_uri, scheme, sizeof(scheme) - 1);
        memcpy(normalized_uri + sizeof(scheme) - 1, cfg->mqtt_uri, uri_len + 1);
        mqtt_uri = normalized_uri;
    }

    build_device_id();

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = mqtt_uri,
        .credentials.username = cfg->mqtt_user,
        .credentials.authentication.password = cfg->mqtt_pass,
        .session.last_will.topic = TOPIC_AVAIL,
        .session.last_will.msg = "offline",
        .session.last_will.qos = 1,
        .session.last_will.retain = true,
        .network.timeout_ms = 10000,
        .network.reconnect_timeout_ms = 5000,
    };

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!s_client) {
        ESP_LOGE(TAG, "esp_mqtt_client_init failed");
        return ESP_FAIL;
    }
    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    if (wifi_manager_is_sta_connected()) {
        start_mqtt_client();
    } else {
        /* Wait for STA to get an IP so the first connect attempt isn't wasted. */
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));
    }
    return ESP_OK;
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

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "ntc1_c", reading.ntc1_temp_c);
    cJSON_AddNumberToObject(root, "ntc2_c", reading.ntc2_temp_c);
    cJSON_AddNumberToObject(root, "fan_pct", fan_control_get_duty_pct());
    cJSON_AddNumberToObject(root, "fan_rpm", fan_control_get_rpm());

    int8_t rssi_dbm;
    if (wifi_manager_get_rssi(&rssi_dbm) == ESP_OK) {
        cJSON_AddNumberToObject(root, "wifi_rssi_dbm", rssi_dbm);
        cJSON_AddNumberToObject(root, "wifi_snr_db", rssi_dbm - WIFI_ASSUMED_NOISE_FLOOR_DBM);
    }

    char *payload = cJSON_PrintUnformatted(root);
    if (payload) {
        esp_mqtt_client_publish(s_client, TOPIC_STATE, payload, 0, 0, false);
        cJSON_free(payload);
    }
    cJSON_Delete(root);
    return ESP_OK;
}

bool mqtt_client_app_is_connected(void)
{
    return s_connected;
}
