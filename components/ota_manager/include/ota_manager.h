#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Marks the currently running app as valid (call once app is confirmed healthy,
 * to prevent automatic rollback on next reboot). */
esp_err_t ota_manager_mark_valid(void);

/* Downloads and applies firmware from an HTTPS URL, then reboots on success. */
esp_err_t ota_manager_update_from_url(const char *url);

/* Writes a single chunk of firmware data received e.g. from an HTTP upload.
 * Call ota_manager_begin_upload() first, then this repeatedly, then
 * ota_manager_finish_upload(). */
esp_err_t ota_manager_begin_upload(void);
esp_err_t ota_manager_write_chunk(const uint8_t *data, size_t len);
esp_err_t ota_manager_finish_upload(void);

#ifdef __cplusplus
}
#endif
