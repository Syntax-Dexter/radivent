idf_component_register(
    SRCS "mqtt_client_app.c"
    INCLUDE_DIRS "include"
    REQUIRES mqtt app_config sensors fan_control
)
