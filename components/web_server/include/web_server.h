#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Starts the HTTP config/status web server (served on both AP and STA IPs). */
esp_err_t web_server_start(void);

#ifdef __cplusplus
}
#endif
