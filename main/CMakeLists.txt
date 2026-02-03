

idf_component_register(
    SRCS 
        "app_main.c"
        "wifi.c"
        "mqtt.c"
        "tasks.c"
        "sensor_manager.c"
        "msg_queue.c"
        "backoff.c"
        "crash_counter.c"
        "command.c"
        "ota.c"
        "utils.c"
        "config_manager.c"
        "mqtt_state.c"
        "MyTest.c"
        "state_manager.c"
    INCLUDE_DIRS "."
    REQUIRES
        freertos
        esp_timer
        esp_wifi
        esp_event
        esp_netif
        driver
        nvs_flash
        mqtt
        json
        app_update
        esp_http_client
        mbedtls
    EMBED_TXTFILES "certs_ca.crt" "cert.pem"
)
