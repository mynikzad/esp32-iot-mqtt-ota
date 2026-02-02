#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
typedef struct {
    uint32_t version;


    char wifi_ssid[32];
    char wifi_pass[64];
    char mqtt_uri[128];
    char mqtt_user[32];
    char mqtt_pass[32];

    uint8_t led_state;

    uint8_t mqtt_enabled;
    uint8_t sensor_enabled;
    uint32_t sample_interval;
    uint32_t crc;
} device_config_t;

void config_init(void);
void config_save(const device_config_t *cfg);
void config_load(device_config_t *cfg);
void config_load_defaults(device_config_t *cfg);
const device_config_t* config_get(void);
void config_set_led_state(int state);
void config_set_sample_interval(uint32_t interval);
void  config_set_sensor_enabled(bool enabled );
//void state_update_motor_power(int power);


#endif
