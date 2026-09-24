#include "thread_manager.h"
#include "app_config.h"
#include "sdkconfig.h"
#include "esp_log.h"

static const char *TAG = "thread_manager";

#if CONFIG_FANCTRL_ENABLE_THREAD && CONFIG_OPENTHREAD_ENABLED

#include "esp_openthread.h"
#include "esp_openthread_lock.h"
#include "esp_openthread_netif_glue.h"
#include "esp_openthread_types.h"
#include "esp_vfs_eventfd.h"

/* NOTE: This is a skeleton bring-up of the ESP OpenThread port. Extend with
 * an OTBR/border-agent-free "Thread SED" node config, and use OpenThread's
 * `otIp6` + CoAP APIs to expose sensor/fan control to a Thread network,
 * mirroring ESP-IDF's ot_cli / ot_sleepy_device examples. */

static esp_err_t init_openthread_radio(void)
{
    esp_openthread_platform_config_t config = {
        .radio_config = { .radio_mode = RADIO_MODE_NATIVE },
        .host_config = { .host_connection_mode = HOST_CONNECTION_MODE_NONE },
        .port_config = { .storage_partition_name = "nvs", .netif_queue_size = 10, .task_queue_size = 10 },
    };
    return esp_openthread_init(&config);
}

esp_err_t thread_manager_init(void)
{
    esp_vfs_eventfd_config_t eventfd_config = { .max_fds = 3 };
    esp_err_t err = esp_vfs_eventfd_register(&eventfd_config);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_vfs_eventfd_register failed: %s", esp_err_to_name(err));
        return err;
    }

    err = init_openthread_radio();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OpenThread init failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGW(TAG, "OpenThread radio initialized - dataset/CoAP endpoints still need to be implemented");
    return ESP_OK;
}

#else

esp_err_t thread_manager_init(void)
{
    ESP_LOGI(TAG, "Thread disabled (CONFIG_FANCTRL_ENABLE_THREAD / CONFIG_OPENTHREAD_ENABLED)");
    return ESP_OK;
}

#endif
