#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initializes ESP-NOW on top of the already-started Wi-Fi driver. */
esp_err_t espnow_manager_init(void);

/* Broadcasts current sensor + fan telemetry to any paired ESP-NOW peers. */
esp_err_t espnow_manager_broadcast_state(void);

#ifdef __cplusplus
}
#endif
