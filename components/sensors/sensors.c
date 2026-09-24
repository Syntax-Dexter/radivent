#include <math.h>
#include <stdbool.h>
#include "sensors.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "sensors";

#define VREF_MV                 3300
#define ADC_ATTEN                ADC_ATTEN_DB_12
#define ADC_BITWIDTH             ADC_BITWIDTH_DEFAULT

#define NTC_NOMINAL_OHM          CONFIG_FANCTRL_NTC_NOMINAL_OHM
#define NTC_BETA                 CONFIG_FANCTRL_NTC_BETA
#define NTC_SERIES_OHM           CONFIG_FANCTRL_NTC_SERIES_OHM
#define NTC_NOMINAL_TEMP_K       298.15f /* 25 C */

static adc_oneshot_unit_handle_t s_adc_handle;
static adc_cali_handle_t s_cali_handle;
static bool s_cali_enabled;

static const adc_channel_t s_channels[2] = {
    (adc_channel_t)CONFIG_FANCTRL_NTC1_ADC_CHANNEL,
    (adc_channel_t)CONFIG_FANCTRL_NTC2_ADC_CHANNEL,
};

static float adc_mv_to_temp_c(int mv)
{
    /* NTC on the low side of a divider: Vout = Vcc * Rntc / (Rntc + Rseries) */
    if (mv <= 0 || mv >= VREF_MV) {
        return NAN;
    }
    float r_ntc = ((float)NTC_SERIES_OHM * mv) / (float)(VREF_MV - mv);
    float steinhart = logf(r_ntc / NTC_NOMINAL_OHM) / NTC_BETA;
    steinhart += 1.0f / NTC_NOMINAL_TEMP_K;
    float temp_k = 1.0f / steinhart;
    return temp_k - 273.15f;
}

esp_err_t sensors_init(void)
{
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    esp_err_t err = adc_oneshot_new_unit(&unit_cfg, &s_adc_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed: %s", esp_err_to_name(err));
        return err;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    for (int i = 0; i < 2; i++) {
        err = adc_oneshot_config_channel(s_adc_handle, s_channels[i], &chan_cfg);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "config_channel[%d] failed: %s", i, esp_err_to_name(err));
            return err;
        }
    }

    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    err = adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_cali_handle);
    s_cali_enabled = (err == ESP_OK);
    if (!s_cali_enabled) {
        ESP_LOGW(TAG, "ADC calibration unavailable, using raw scaling");
    }

    ESP_LOGI(TAG, "Sensors initialized (NTC1 ch%d, NTC2 ch%d)", s_channels[0], s_channels[1]);
    return ESP_OK;
}

static esp_err_t read_channel_mv(adc_channel_t chan, int *mv_out)
{
    int raw;
    esp_err_t err = adc_oneshot_read(s_adc_handle, chan, &raw);
    if (err != ESP_OK) {
        return err;
    }
    if (s_cali_enabled) {
        return adc_cali_raw_to_voltage(s_cali_handle, raw, mv_out);
    }
    /* Fallback: assume 12-bit resolution linear scaling against Vref. */
    *mv_out = (raw * VREF_MV) / 4095;
    return ESP_OK;
}

esp_err_t sensors_read(sensors_reading_t *out)
{
    int mv1, mv2;
    esp_err_t err = read_channel_mv(s_channels[0], &mv1);
    if (err != ESP_OK) return err;
    err = read_channel_mv(s_channels[1], &mv2);
    if (err != ESP_OK) return err;

    out->ntc1_temp_c = adc_mv_to_temp_c(mv1);
    out->ntc2_temp_c = adc_mv_to_temp_c(mv2);
    return ESP_OK;
}
