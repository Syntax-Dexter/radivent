idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash app_config sensors fan_control led_indicator
                             wifi_manager web_server ota_manager mqtt_client_app
                             espnow_manager zigbee_manager thread_manager)
