#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float ntc1_temp_c;
    float ntc2_temp_c;
} sensors_reading_t;

/* Initializes the ADC unit + channels used for the two NTC thermistor dividers. */
esp_err_t sensors_init(void);

/* Samples both NTC channels and converts to degrees Celsius via the Beta equation. */
esp_err_t sensors_read(sensors_reading_t *out);

#ifdef __cplusplus
}
#endif
