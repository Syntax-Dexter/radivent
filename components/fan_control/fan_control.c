#include "fan_control.h"
#include "app_config.h"
#include "sdkconfig.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "fan_control";

#define FAN_PWM_TIMER            LEDC_TIMER_0
#define FAN_PWM_MODE             LEDC_LOW_SPEED_MODE
#define FAN_PWM_CHANNEL          LEDC_CHANNEL_0
#define FAN_PWM_FREQ_HZ          25000 /* 25 kHz, above audible range for MOSFET switching */
#define FAN_PWM_RES              LEDC_TIMER_10_BIT
#define FAN_PWM_MAX_DUTY         ((1 << 10) - 1)

static uint8_t s_current_duty_pct;

esp_err_t fan_control_init(void)
{
    ledc_timer_config_t timer_cfg = {
        .speed_mode = FAN_PWM_MODE,
        .timer_num = FAN_PWM_TIMER,
        .duty_resolution = FAN_PWM_RES,
        .freq_hz = FAN_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t err = ledc_timer_config(&timer_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_timer_config failed: %s", esp_err_to_name(err));
        return err;
    }

    ledc_channel_config_t chan_cfg = {
        .gpio_num = CONFIG_FANCTRL_PWM_GPIO,
        .speed_mode = FAN_PWM_MODE,
        .channel = FAN_PWM_CHANNEL,
        .timer_sel = FAN_PWM_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    err = ledc_channel_config(&chan_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_channel_config failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Fan PWM initialized on GPIO%d @ %d Hz", CONFIG_FANCTRL_PWM_GPIO, FAN_PWM_FREQ_HZ);
    return fan_control_set_duty_pct(0);
}

esp_err_t fan_control_set_duty_pct(uint8_t duty_pct)
{
    app_config_t *cfg = app_config_get();
    if (duty_pct > 100) duty_pct = 100;
    if (duty_pct != 0) {
        if (duty_pct < cfg->fan_min_duty_pct) duty_pct = cfg->fan_min_duty_pct;
        if (duty_pct > cfg->fan_max_duty_pct) duty_pct = cfg->fan_max_duty_pct;
    }

    uint32_t duty = (FAN_PWM_MAX_DUTY * duty_pct) / 100;
    esp_err_t err = ledc_set_duty(FAN_PWM_MODE, FAN_PWM_CHANNEL, duty);
    if (err != ESP_OK) return err;
    err = ledc_update_duty(FAN_PWM_MODE, FAN_PWM_CHANNEL);
    if (err != ESP_OK) return err;

    s_current_duty_pct = duty_pct;
    return ESP_OK;
}

uint8_t fan_control_get_duty_pct(void)
{
    return s_current_duty_pct;
}
