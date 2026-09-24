#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LED_PATTERN_OFF,
    LED_PATTERN_ON,
    LED_PATTERN_SLOW_BLINK,   /* 1 Hz, e.g. idle / AP mode */
    LED_PATTERN_FAST_BLINK,   /* 5 Hz, e.g. error / OTA in progress */
    LED_PATTERN_DOUBLE_PULSE, /* two quick pulses then pause, e.g. connected+running */
} led_pattern_t;

/* Initializes the status LED GPIO and starts the background blink task. */
esp_err_t led_indicator_init(void);

/* Changes the active blink pattern (thread-safe). */
void led_indicator_set_pattern(led_pattern_t pattern);

#ifdef __cplusplus
}
#endif
