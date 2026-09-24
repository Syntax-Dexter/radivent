#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initializes an OpenThread stack instance (FTD/MTD) on the 802.15.4 radio.
 * No-op unless CONFIG_FANCTRL_ENABLE_THREAD is set. Also requires
 * CONFIG_OPENTHREAD_ENABLED=y (see sdkconfig.defaults) so the "openthread"
 * component + esp_openthread APIs are built. */
esp_err_t thread_manager_init(void);

#ifdef __cplusplus
}
#endif
