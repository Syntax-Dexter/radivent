#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initializes the Zigbee stack (End Device) exposing an On/Off + analog fan
 * cluster. No-op unless CONFIG_FANCTRL_ENABLE_ZIGBEE is set.
 *
 * Requires the managed component in main/idf_component.yml:
 *   dependencies:
 *     espressif/esp-zigbee-lib: "*"
 *     espressif/esp-zboss-lib: "*"
 * and CONFIG_ZB_ENABLED / CONFIG_ZB_ZED via menuconfig. */
esp_err_t zigbee_manager_init(void);

#ifdef __cplusplus
}
#endif
