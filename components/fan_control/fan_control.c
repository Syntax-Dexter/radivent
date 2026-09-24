#include "fan_control.h"
#include "app_config.h"
#include "board_config.h"
#include "driver/ledc.h"
#include "driver/pulse_cnt.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "fan_control";

#define FAN_PWM_TIMER            LEDC_TIMER_0
#define FAN_PWM_MODE             LEDC_LOW_SPEED_MODE
#define FAN_PWM_CHANNEL          LEDC_CHANNEL_0
#define FAN_PWM_FREQ_HZ          25000 /* 25 kHz, above audible range for MOSFET switching */
#define FAN_PWM_RES              LEDC_TIMER_10_BIT
#define FAN_PWM_MAX_DUTY         ((1 << 10) - 1)

#define TACH_PULSES_PER_REV      2       /* typical 2-pulse/revolution PC fan tach */
#define TACH_SAMPLE_PERIOD_US    1000000 /* 1 s sampling window */
#define TACH_PCNT_HIGH_LIMIT     32767
#define TACH_PCNT_LOW_LIMIT      (-32768)

static uint8_t s_current_duty_pct;
static volatile uint16_t s_current_rpm;
static pcnt_unit_handle_t s_tach_unit;

static void tach_sample_cb(void *arg)
{
    int count = 0;
    pcnt_unit_get_count(s_tach_unit, &count);
    pcnt_unit_clear_count(s_tach_unit);
    if (count < 0) count = 0;
    s_current_rpm = (uint16_t)((count * 60) / TACH_PULSES_PER_REV);
}

static esp_err_t tach_init(void)
{
    pcnt_unit_config_t unit_cfg = {
        .high_limit = TACH_PCNT_HIGH_LIMIT,
        .low_limit = TACH_PCNT_LOW_LIMIT,
    };
    esp_err_t err = pcnt_new_unit(&unit_cfg, &s_tach_unit);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "pcnt_new_unit failed: %s", esp_err_to_name(err));
        return err;
    }

    pcnt_glitch_filter_config_t filter_cfg = {
        .max_glitch_ns = 1000,
    };
    pcnt_unit_set_glitch_filter(s_tach_unit, &filter_cfg);

    pcnt_chan_config_t chan_cfg = {
        .edge_gpio_num = TACH_READ_GPIO,
        .level_gpio_num = -1,
    };
    pcnt_channel_handle_t chan = NULL;
    err = pcnt_new_channel(s_tach_unit, &chan_cfg, &chan);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "pcnt_new_channel failed: %s", esp_err_to_name(err));
        return err;
    }
    pcnt_channel_set_edge_action(chan, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD);

    err = pcnt_unit_enable(s_tach_unit);
    if (err != ESP_OK) return err;
    err = pcnt_unit_clear_count(s_tach_unit);
    if (err != ESP_OK) return err;
    err = pcnt_unit_start(s_tach_unit);
    if (err != ESP_OK) return err;

    const esp_timer_create_args_t timer_args = {
        .callback = tach_sample_cb,
        .name = "tach_sample",
    };
    esp_timer_handle_t timer;
    err = esp_timer_create(&timer_args, &timer);
    if (err != ESP_OK) return err;
    err = esp_timer_start_periodic(timer, TACH_SAMPLE_PERIOD_US);
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "Tach input initialized on GPIO%d", TACH_READ_GPIO);
    return ESP_OK;
}

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
        .gpio_num = FAN_PWM_GPIO,
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

    err = tach_init();
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "Fan PWM initialized on GPIO%d @ %d Hz", FAN_PWM_GPIO, FAN_PWM_FREQ_HZ);
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

uint16_t fan_control_get_rpm(void)
{
    return s_current_rpm;
}
