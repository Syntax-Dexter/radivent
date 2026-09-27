idf_component_register(
    SRCS "rgb_led.c"
    INCLUDE_DIRS "include"
    REQUIRES board_config led_strip
)
