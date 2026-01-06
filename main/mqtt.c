// MQTT message callback
// This function is called by the MQTT client library when a message arrives.
// It must stay lightweight and only forward commands to the message queue.
#include "esp_ota_ops.h"
#include "mqtt.h"
#include "config.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/gpio.h"
#include "backoff.h"
#include "crash_counter.h"
#include "cJSON.h"
#include "command.h"
#include "utils.h"
#include "config_manager.h"
#include "mqtt_state.h"
extern const uint8_t certs_ca_crt_start[] asm("_binary_certs_ca_crt_start");
extern const uint8_t certs_ca_crt_end[] asm("_binary_certs_ca_crt_end");
static volatile bool need_reconnect = false;
static const char *TAG = "MQTT";
static volatile bool mqtt_connected = false;
static esp_mqtt_client_handle_t client = NULL;

// Keep MQTT callback lightweight to avoid blocking the client loop
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch (event->event_id)
    {
    case MQTT_EVENT_BEFORE_CONNECT:
        ESP_LOGI(TAG, "MQTT before connect");
        break;
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "-----ROLLBACK CANCEL-2-----");
        esp_ota_mark_app_valid_cancel_rollback();
        ESP_LOGI(TAG, "MQTT connected");
        crash_counter_reset();
        esp_mqtt_client_subscribe(event->client, "device/esp32-01/cmd", 0);
        esp_mqtt_client_subscribe(event->client, "device/esp32-01/ctrl/#", 0); // future
        esp_mqtt_client_subscribe(event->client, "adel/led", 0);
        backoff_reset();
        mqtt_connected = true;
        mqtt_state_publish();
        break;

    case MQTT_EVENT_DATA:
    {
        // Incoming MQTT commands are handled directly here
        // Commands are lightweight and do not use queues

        if (event->topic_len <= 0)
        {
            ESP_LOGW(TAG, "Invalid MQTT message: missing topic");
            break;
        }
        else if (event->data_len <= 0)
        {
            ESP_LOGW(TAG, "Invalid MQTT message: missing payload");
            break;
        }
        char topic[event->topic_len + 1];
        memcpy(topic, event->topic, event->topic_len);
        topic[event->topic_len] = '\0';
        char *payload = strndup(event->data, event->data_len);
        if (!payload)
            break;

        if (strcmp(topic, "device/esp32-01/cmd") == 0)
        {
            handle_command_json(payload, "device/esp32-01/ack");
        }
        else if (strcmp(topic, "adel/led") == 0)
        {
            handle_command_json(payload, "adel/ack");
        }
        free(payload);
        break;
    }
    case MQTT_EVENT_DISCONNECTED:
        mqtt_connected = false;
        ESP_LOGE(TAG, "MQTT disconnected (network/TLS/broker issue)");
        break;

    /*   mqtt_connected = false;
        need_reconnect = true; // فقط فلگ
        break;*/
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT generic error");
        break;

    default:
        break;
    }
}

void mqtt_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = MQTT_URI,
            .verification.certificate = (const char *)certs_ca_crt_start,
        },
        .credentials = {
            .username = MQTT_USER,
            .authentication.password = MQTT_PASS,
        },
        .session = {.keepalive = 120, .disable_clean_session = false}};

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
}

void mqtt_start(void)
{
    if (client)
        esp_mqtt_client_start(client);
}
bool mqtt_is_connected(void)
{
    return mqtt_connected;
}

void mqtt_publish_status(const char *msg)
{
    if (mqtt_connected)
    {
        esp_mqtt_client_publish(client, "adel/status", msg, 0, 1, 0);
    }
}
void mqtt_request_reconnect(void)
{
    need_reconnect = true;
}
void mqtt_try_reconnect(void)
{
    if (!mqtt_connected && client)
    {
        ESP_LOGW(TAG, "Trying reconnect...");
        esp_mqtt_client_reconnect(client);
    }
}
void mqtt_publish_sensor_data(const char *json_str)
{
    if (mqtt_is_connected() && client)
    {
        // topic را مطابق استاندارد پروژه‌ات انتخاب کن
        esp_mqtt_client_publish(client, "adel/sensors", json_str, 0, 1, 0);
    }
    else
    {
        ESP_LOGW(TAG, "mqtt publish skipped (not connected)");
    }
}
// mqtt.c
void mqtt_publish(const char *topic, const char *payload)
{
    if (mqtt_get_client() && mqtt_connected)
    {
        // esp_mqtt_client_publish(mqtt_client_handle, topic, payload, 0, 0, 0);
        esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
    }
    else
    {
        ESP_LOGW("MQTT_PUB", "Not connected, drop publish");
    }
}
esp_mqtt_client_handle_t mqtt_get_client(void)
{
    return client; // اسم کلاینت تو
}
void mqtt_set_led(int v)
{
    gpio_set_level(LED_GPIO, v ? 1 : 0);
    config_set_led_state(v);
    config_save(config_get());
    ESP_LOGI("LED", "Setting LED to %d", v);
    mqtt_state_publish();
}
