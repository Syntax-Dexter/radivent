idf_component_register(
    SRCS "fan_control.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_ledc esp_driver_pcnt esp_timer app_config board_config
)
