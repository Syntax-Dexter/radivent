#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Brings up Wi-Fi in APSTA mode: SoftAP always on at 192.168.4.1 for local
 * configuration, plus STA connecting to the configured home network (if any). */
esp_err_t wifi_manager_init(void);

/* True once the STA interface has obtained an IP address. */
bool wifi_manager_is_sta_connected(void);

/* Assumed Wi-Fi noise floor in dBm, used to approximate SNR from RSSI since
 * the ESP32 radio does not expose a real noise-floor measurement. */
#define WIFI_ASSUMED_NOISE_FLOOR_DBM (-95)

/* Reads the current STA RSSI in dBm. Returns ESP_ERR_INVALID_STATE if not connected. */
esp_err_t wifi_manager_get_rssi(int8_t *rssi_dbm);

#ifdef __cplusplus
}
#endif
