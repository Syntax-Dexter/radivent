#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Connects to the broker configured in app_config and starts publishing
 * periodic sensor/fan state, subscribing to the fan speed command topic. */
esp_err_t mqtt_client_app_init(void);

/* Publishes current sensor + fan telemetry as a single JSON payload. */
esp_err_t mqtt_client_app_publish_state(void);

#ifdef __cplusplus
}
#endif
