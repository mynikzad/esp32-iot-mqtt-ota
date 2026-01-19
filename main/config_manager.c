#include "config_manager.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include "esp_log.h"
#include <inttypes.h>
#include "mqtt_state.h"
#include "sdkconfig.h"

#define CONFIG_NAMESPACE "storage"
#define CONFIG_VERSION 1

device_config_t g_config;
// یک CRC ساده برای بررسی سالم بودن داده
static uint32_t calc_crc(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
        {
            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }
    return ~crc;
}

void config_load_defaults(device_config_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));

    cfg->version = CONFIG_VERSION;
    // READ From Menu Config

    strcpy(cfg->wifi_ssid, CONFIG_WIFI_SSID);
    strcpy(cfg->wifi_pass, CONFIG_WIFI_PASSWORD);

    strcpy(cfg->mqtt_uri, "mqtts://192.168.1.3:8883");
    strcpy(cfg->mqtt_user, "AliUser");
    strcpy(cfg->mqtt_pass, "123");

    cfg->led_state = 0;

    cfg->mqtt_enabled = 1;
    cfg->sensor_enabled = 0;
    cfg->sample_interval = 5000;

    cfg->crc = calc_crc((uint8_t *)cfg, sizeof(device_config_t) - sizeof(uint32_t));
}

void config_save(const device_config_t *cfg)
{
    nvs_handle_t handle;
    if (nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK)
        return;

    nvs_set_blob(handle, "config", cfg, sizeof(device_config_t));
    nvs_commit(handle);
    nvs_close(handle);
}

void config_load(device_config_t *cfg)
{
    nvs_handle_t handle;
    size_t size = sizeof(device_config_t);

    if (nvs_open(CONFIG_NAMESPACE, NVS_READONLY, &handle) != ESP_OK)
    {
        ESP_LOGW("CONFIG", "NVS open failed → loading defaults");
        //TODO : Check for nvs_close(handle) need of not  .just one time  some issu happend if used 
        config_load_defaults(cfg);
        return;
    }

    if (nvs_get_blob(handle, "config", cfg, &size) != ESP_OK)
    {
        ESP_LOGW("CONFIG", "No config blob → loading defaults");
        nvs_close(handle);
        config_load_defaults(cfg);
        return;
    }

    nvs_close(handle);

    uint32_t crc_calc = calc_crc((uint8_t *)cfg, sizeof(device_config_t) - sizeof(uint32_t));

    ESP_LOGI("CONFIG", "CRC calc done");

    // ESP_LOGI("CONFIG", "Stored CRC = 0x%08X", cfg->crc);
    // ESP_LOGI("CONFIG", "Calc CRC = %d", crc_calc);

    // ESP_LOGI("CONFIG", "Version stored=%d expected=%d", cfg->version, CONFIG_VERSION);

    if (crc_calc != cfg->crc || cfg->version != CONFIG_VERSION)
    {
        ESP_LOGE("CONFIG", "CRC mismatch OR Version");
        config_load_defaults(cfg);
        config_save(cfg);
    }
}

void config_set_led_state(int state)
{
    g_config.led_state = state;
    g_config.crc = calc_crc((uint8_t *)&g_config, sizeof(device_config_t) - sizeof(uint32_t));
}

void config_init(void)
{
    nvs_flash_init();
    config_load(&g_config);
    mqtt_state_publish();
}

const device_config_t *config_get(void)
{
    return &g_config;
}
