#include "mqtt_state.h"
#include "mqtt.h"
#include "config.h"
#include "cJSON.h"
#include "esp_log.h"
#include "config_manager.h"
static const char *TAG = "MQTT_STATE";

void mqtt_state_init(void)
{
    ESP_LOGI(TAG, "state layer init");
}

void mqtt_state_publish(void)
{

    esp_mqtt_client_handle_t client = mqtt_get_client();
    if (!client || !mqtt_is_connected())
    {
        ESP_LOGW(TAG, "MQTT not ready, skipping publish");
        return;
    }
    // 1) جمع کردن state از config/NVS
    int led = config_get()->led_state;
    int sensor_enabled = config_get()->sensor_enabled;
    int sample_rate = config_get()->sample_interval;
    // 2) ساخت JSON
    cJSON *root = cJSON_CreateObject();
    if (!root)
        return;

    cJSON_AddNumberToObject(root, "led", led);
    cJSON_AddNumberToObject(root, "sensor_enabled", sensor_enabled);
    cJSON_AddNumberToObject(root, "sample_rate", sample_rate);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json)
        return;

    // 3) publish retained
    esp_mqtt_client_publish(mqtt_get_client(), "device/esp32-01/state", json, 0, 1, 1);
    // Que-  retained = true

    ESP_LOGI(TAG, "state published: %s", json);
    free(json);
}
