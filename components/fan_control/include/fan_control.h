#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initializes the LEDC PWM timer/channel driving the MOSFET gate. */
esp_err_t fan_control_init(void);

/* Sets fan speed as a duty percentage (0-100), clamped to configured min/max. */
esp_err_t fan_control_set_duty_pct(uint8_t duty_pct);

/* Returns the last applied duty percentage. */
uint8_t fan_control_get_duty_pct(void);

#ifdef __cplusplus
}
#endif
