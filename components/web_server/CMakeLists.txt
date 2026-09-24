idf_component_register(
    SRCS "web_server.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "."
    REQUIRES esp_http_server app_config sensors fan_control ota_manager cjson
)
