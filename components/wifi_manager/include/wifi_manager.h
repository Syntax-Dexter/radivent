#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Brings up Wi-Fi in APSTA mode: SoftAP always on at 192.168.4.1 for local
 * configuration, plus STA connecting to the configured home network (if any). */
esp_err_t wifi_manager_init(void);

/* True once the STA interface has obtained an IP address. */
bool wifi_manager_is_sta_connected(void);

#ifdef __cplusplus
}
#endif
