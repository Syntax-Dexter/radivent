#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Default brightness out of 255, applied to all colors until changed. */
#define RGB_LED_DEFAULT_BRIGHTNESS 20

/* Initializes the RMT-driven addressable RGB LED (WS2812-compatible), off by default. */
esp_err_t rgb_led_init(void);

/* Queues a one-shot blink sequence: blinks the given color `times` (500ms on/500ms
 * off), then leaves the LED off. Sequences are queued and run one at a time. */
esp_err_t rgb_led_flash(uint8_t r, uint8_t g, uint8_t b, uint8_t times);

/* Changes the global brightness (0-255) applied to subsequent flashes. */
esp_err_t rgb_led_set_brightness(uint8_t brightness);

#ifdef __cplusplus
}
#endif
