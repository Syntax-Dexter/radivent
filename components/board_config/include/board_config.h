#pragma once

/* Central GPIO pin map for the radiator fan controller board.
 * Single source of truth for all pin assignments used across components. */

#define STATUS_LED_GPIO      10
#define NTC1_ADC_GPIO         2
#define NTC2_ADC_GPIO         3
#define I2C_SDA_GPIO          6
#define I2C_SCL_GPIO          7
#define UART_TX_GPIO         16
#define UART_RX_GPIO         17
#define FAN_PWM_GPIO         18
#define TACH_READ_GPIO       20
