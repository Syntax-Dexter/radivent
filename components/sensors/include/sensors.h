#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float ntc1_temp_c;
    float ntc2_temp_c;
} sensors_reading_t;

/* Initializes the ADC unit + channels used for the two NTC thermistor dividers,
 * plus the shared I2C bus on SDA/SCL for future I2C sensors. */
esp_err_t sensors_init(void);

/* Samples both NTC channels and converts to degrees Celsius via the Beta equation. */
esp_err_t sensors_read(sensors_reading_t *out);

/* Returns the shared I2C bus handle so other components can attach I2C devices. */
i2c_master_bus_handle_t sensors_i2c_bus_handle(void);

#ifdef __cplusplus
}
#endif
