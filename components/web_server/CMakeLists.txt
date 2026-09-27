idf_component_register(
    SRCS "web_server.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "."
    REQUIRES esp_http_server esp_netif esp_wifi app_config sensors fan_control mqtt_client_app ota_manager cjson
)
