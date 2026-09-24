#include "zigbee_manager.h"
#include "app_config.h"
#include "sdkconfig.h"
#include "esp_log.h"

static const char *TAG = "zigbee_manager";

#if CONFIG_FANCTRL_ENABLE_ZIGBEE

/* NOTE: This is a skeleton. Building this branch requires adding the
 * espressif/esp-zigbee-lib managed component (idf_component.yml) which
 * provides esp_zigbee_core.h and the zboss stack. Replace this body with a
 * real Zigbee End Device (Home Automation profile) exposing:
 *   - HA On/Off cluster    -> fan on/off
 *   - HA Level Control     -> fan_control_set_duty_pct()
 *   - HA Temperature Meas. -> sensors_read() (NTC1/NTC2)
 * following ESP-IDF's zigbee HA examples (light_bulb / HA_temperature_sensor). */

esp_err_t zigbee_manager_init(void)
{
    ESP_LOGW(TAG, "Zigbee enabled in Kconfig but not implemented yet - add "
                  "espressif/esp-zigbee-lib and implement the HA endpoint here");
    return ESP_ERR_NOT_SUPPORTED;
}

#else

esp_err_t zigbee_manager_init(void)
{
    ESP_LOGI(TAG, "Zigbee disabled (CONFIG_FANCTRL_ENABLE_ZIGBEE=n)");
    return ESP_OK;
}

#endif /* CONFIG_FANCTRL_ENABLE_ZIGBEE */
