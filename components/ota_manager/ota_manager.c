#include "ota_manager.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"

static const char *TAG = "ota_manager";

static esp_ota_handle_t s_ota_handle;
static const esp_partition_t *s_update_partition;

esp_err_t ota_manager_mark_valid(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    if (esp_ota_get_state_partition(running, &state) == ESP_OK && state == ESP_OTA_IMG_PENDING_VERIFY) {
        ESP_LOGI(TAG, "Marking running app as valid, cancelling rollback");
        return esp_ota_mark_app_valid_cancel_rollback();
    }
    return ESP_OK;
}

esp_err_t ota_manager_update_from_url(const char *url)
{
    esp_http_client_config_t http_cfg = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .keep_alive_enable = true,
    };
    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    ESP_LOGI(TAG, "Starting HTTPS OTA from %s", url);
    esp_err_t err = esp_https_ota(&ota_cfg);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "OTA success, rebooting");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t ota_manager_begin_upload(void)
{
    s_update_partition = esp_ota_get_next_update_partition(NULL);
    if (!s_update_partition) {
        ESP_LOGE(TAG, "No OTA update partition available");
        return ESP_FAIL;
    }
    esp_err_t err = esp_ota_begin(s_update_partition, OTA_SIZE_UNKNOWN, &s_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t ota_manager_write_chunk(const uint8_t *data, size_t len)
{
    return esp_ota_write(s_ota_handle, data, len);
}

esp_err_t ota_manager_finish_upload(void)
{
    esp_err_t err = esp_ota_end(s_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
        return err;
    }
    err = esp_ota_set_boot_partition(s_update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "OTA upload complete, rebooting");
    esp_restart();
    return ESP_OK;
}
