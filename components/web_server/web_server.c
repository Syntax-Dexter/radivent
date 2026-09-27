#include <string.h>
#include <stdlib.h>
#include "web_server.h"
#include "web_server_html.h"
#include "app_config.h"
#include "sensors.h"
#include "fan_control.h"
#include "mqtt_client_app.h"
#include "ota_manager.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "web_server";

static void mqtt_uri_details(const char *uri, char *address, size_t address_len, uint16_t *port)
{
    strlcpy(address, "Not configured", address_len);
    *port = 0;
    if (!uri || uri[0] == '\0') {
        return;
    }

    /* Extract the broker authority while leaving credentials and paths out of the UI. */
    const char *authority = strstr(uri, "://");
    authority = authority ? authority + 3 : uri;
    const char *authority_end = strpbrk(authority, "/?#");
    size_t authority_len = authority_end ? (size_t)(authority_end - authority) : strlen(authority);
    if (authority_len == 0) {
        return;
    }

    uint16_t default_port = strncmp(uri, "mqtts://", 8) == 0 ? 8883 : 1883;
    const char *port_separator = NULL;
    if (authority[0] == '[') {
        const char *closing_bracket = memchr(authority, ']', authority_len);
        if (closing_bracket && (size_t)(closing_bracket - authority + 1) < authority_len &&
            closing_bracket[1] == ':') {
            port_separator = closing_bracket + 1;
        }
    } else {
        const char *separator = memchr(authority, ':', authority_len);
        if (separator) {
            port_separator = separator;
        }
    }

    size_t address_size = port_separator ? (size_t)(port_separator - authority) : authority_len;
    if (address_size >= address_len) {
        address_size = address_len - 1;
    }
    memcpy(address, authority, address_size);
    address[address_size] = '\0';
    *port = default_port;
    if (port_separator) {
        char *end = NULL;
        unsigned long parsed_port = strtoul(port_separator + 1, &end, 10);
        if (end == authority + authority_len && parsed_port <= UINT16_MAX) {
            *port = (uint16_t)parsed_port;
        }
    }
}

static esp_err_t index_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t status_get_handler(httpd_req_t *req)
{
    sensors_reading_t reading = {0};
    sensors_read(&reading);

    app_config_t *cfg = app_config_get();
    char broker_address[APP_CONFIG_URI_MAX_LEN] = {0};
    uint16_t broker_port = 0;
    mqtt_uri_details(cfg->mqtt_uri, broker_address, sizeof(broker_address), &broker_port);

    /* Read the live station details rather than the configured SSID. */
    char connected_ssid[sizeof(((wifi_ap_record_t *)0)->ssid) + 1] = "Not connected";
    char ip_address[16] = "Not assigned";
    wifi_ap_record_t ap_info = {0};
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        /* The driver SSID buffer may use all 32 bytes and omit a terminator. */
        memcpy(connected_ssid, ap_info.ssid, sizeof(ap_info.ssid));
        connected_ssid[sizeof(ap_info.ssid)] = '\0';
    }

    esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t ip_info = {0};
    if (sta_netif && esp_netif_get_ip_info(sta_netif, &ip_info) == ESP_OK && ip_info.ip.addr != 0) {
        esp_ip4addr_ntoa(&ip_info.ip, ip_address, sizeof(ip_address));
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "ntc1_c", reading.ntc1_temp_c);
    cJSON_AddNumberToObject(root, "ntc2_c", reading.ntc2_temp_c);
    cJSON_AddNumberToObject(root, "fan_pct", fan_control_get_duty_pct());
    cJSON_AddStringToObject(root, "connected_ssid", connected_ssid);
    cJSON_AddStringToObject(root, "ip_address", ip_address);
    cJSON_AddStringToObject(root, "mqtt_address", broker_address);
    cJSON_AddNumberToObject(root, "mqtt_port", broker_port);
    cJSON_AddBoolToObject(root, "mqtt_connected", mqtt_client_app_is_connected());
    char *json = cJSON_PrintUnformatted(root);

    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_sendstr(req, json);
    free(json);
    cJSON_Delete(root);
    return err;
}

static esp_err_t config_get_handler(httpd_req_t *req)
{
    app_config_t *cfg = app_config_get();
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "sta_ssid", cfg->sta_ssid);
    cJSON_AddStringToObject(root, "mqtt_uri", cfg->mqtt_uri);
    cJSON_AddStringToObject(root, "mqtt_user", cfg->mqtt_user);
    cJSON_AddStringToObject(root, "ota_url", cfg->ota_url);
    cJSON_AddBoolToObject(root, "zb_en", cfg->zigbee_enabled);
    cJSON_AddBoolToObject(root, "th_en", cfg->thread_enabled);
    cJSON_AddBoolToObject(root, "en_en", cfg->espnow_enabled);

    char *json = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_sendstr(req, json);
    free(json);
    cJSON_Delete(root);
    return err;
}

static esp_err_t read_request_body(httpd_req_t *req, char *buf, size_t buf_len)
{
    int total = req->content_len;
    if (total <= 0 || (size_t)total >= buf_len) {
        return ESP_ERR_INVALID_SIZE;
    }
    int received = 0;
    while (received < total) {
        int r = httpd_req_recv(req, buf + received, total - received);
        if (r <= 0) {
            return ESP_FAIL;
        }
        received += r;
    }
    buf[received] = '\0';
    return ESP_OK;
}

static void set_str_field(cJSON *root, const char *key, char *dst, size_t dst_len)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (cJSON_IsString(item) && item->valuestring) {
        strlcpy(dst, item->valuestring, dst_len);
    }
}

static void set_nonempty_str_field(cJSON *root, const char *key, char *dst, size_t dst_len)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    /* Password inputs are blank when the existing secret is intentionally kept. */
    if (cJSON_IsString(item) && item->valuestring && item->valuestring[0] != '\0') {
        strlcpy(dst, item->valuestring, dst_len);
    }
}

static esp_err_t config_post_handler(httpd_req_t *req)
{
    char buf[512];
    if (read_request_body(req, buf, sizeof(buf)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad body");
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad JSON");
        return ESP_FAIL;
    }

    app_config_t *cfg = app_config_get();
    set_str_field(root, "sta_ssid", cfg->sta_ssid, sizeof(cfg->sta_ssid));
    set_nonempty_str_field(root, "sta_pass", cfg->sta_pass, sizeof(cfg->sta_pass));
    set_str_field(root, "mqtt_uri", cfg->mqtt_uri, sizeof(cfg->mqtt_uri));
    set_str_field(root, "mqtt_user", cfg->mqtt_user, sizeof(cfg->mqtt_user));
    set_nonempty_str_field(root, "mqtt_pass", cfg->mqtt_pass, sizeof(cfg->mqtt_pass));
    set_str_field(root, "ota_url", cfg->ota_url, sizeof(cfg->ota_url));

    cJSON *item;
    if ((item = cJSON_GetObjectItemCaseSensitive(root, "zb_en"))) cfg->zigbee_enabled = cJSON_IsTrue(item);
    if ((item = cJSON_GetObjectItemCaseSensitive(root, "th_en"))) cfg->thread_enabled = cJSON_IsTrue(item);
    if ((item = cJSON_GetObjectItemCaseSensitive(root, "en_en"))) cfg->espnow_enabled = cJSON_IsTrue(item);

    cJSON_Delete(root);

    esp_err_t err = app_config_save();
    httpd_resp_sendstr(req, "{\"ok\":true}");

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Config saved, rebooting to apply");
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
    }
    return ESP_OK;
}

static esp_err_t ota_post_handler(httpd_req_t *req)
{
    char buf[256];
    if (read_request_body(req, buf, sizeof(buf)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad body");
        return ESP_FAIL;
    }
    cJSON *root = cJSON_Parse(buf);
    cJSON *url_item = root ? cJSON_GetObjectItemCaseSensitive(root, "url") : NULL;
    if (!cJSON_IsString(url_item)) {
        if (root) cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing url");
        return ESP_FAIL;
    }

    app_config_t *cfg = app_config_get();
    strlcpy(cfg->ota_url, url_item->valuestring, sizeof(cfg->ota_url));
    app_config_save();
    cJSON_Delete(root);

    httpd_resp_sendstr(req, "{\"ok\":true}");
    /* Perform the OTA update from a background task so the HTTP response can flush first. */
    ota_manager_update_from_url(cfg->ota_url);
    return ESP_OK;
}

esp_err_t web_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    httpd_handle_t server = NULL;
    esp_err_t err = httpd_start(&server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        return err;
    }

    static const httpd_uri_t routes[] = {
        { .uri = "/", .method = HTTP_GET, .handler = index_get_handler },
        { .uri = "/api/status", .method = HTTP_GET, .handler = status_get_handler },
        { .uri = "/api/config", .method = HTTP_GET, .handler = config_get_handler },
        { .uri = "/api/config", .method = HTTP_POST, .handler = config_post_handler },
        { .uri = "/api/ota", .method = HTTP_POST, .handler = ota_post_handler },
    };
    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
        httpd_register_uri_handler(server, &routes[i]);
    }

    ESP_LOGI(TAG, "Web server started");
    return ESP_OK;
}
