#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stdint.h>

typedef struct {
    uint32_t version;
    uint32_t crc;

    char wifi_ssid[32];
    char wifi_pass[64];
    char mqtt_uri[128];
    char mqtt_user[32];
    char mqtt_pass[32];

    uint8_t mqtt_enabled;
    uint8_t sensor_enabled;
    uint32_t sample_interval;

} device_config_t;

void config_init(void);
void config_save(const device_config_t *cfg);
void config_load(device_config_t *cfg);
void config_load_defaults(device_config_t *cfg);
const device_config_t* config_get(void);

#endif
