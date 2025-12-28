#pragma once
#include <stdbool.h>
#include "mqtt_client.h"
void mqtt_init(void);
void mqtt_start(void);
bool mqtt_is_connected(void);
void mqtt_publish_status(const char *msg);
//void mqtt_publish_sensor_data(const char *json); // اگر از این استفاده کردی
void mqtt_publish_sensor_data(const char *json_str);
void mqtt_request_reconnect(void);
void mqtt_try_reconnect(void);
// mqtt.h
void mqtt_publish(const char *topic, const char *payload);
void mqtt_set_led(int v); // wrapper to set GPIO or send internal event
esp_mqtt_client_handle_t mqtt_get_client(void);



