// commands.c

#include "esp_log.h"
#include "cJSON.h"
#include "mqtt.h"
#include "sensor_manager.h"
#include "crash_counter.h"
#include "esp_system.h"
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "nvs_flash.h"
#include "ota.h"
// تو قبلا اضافه کردی ولی مطمئن شو هست
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "mqtt_state.h"
#include "MyTest.h"
#include "state_manager.h"

static bool actuator_busy = false;
static int dynamic_log_level = ESP_LOG_INFO;
static const char *TAG = "COMMANDS";
static bool device_busy = false;
// helper: publish ack (topic is e.g. device/esp32-01/ack)
static void publish_ack(const char *reply_topic, const char *status, const char *cmd, const char *req_id, cJSON *extra)
{
    cJSON *root = cJSON_CreateObject();
    if (!root)
        return;

    if (req_id)
        cJSON_AddStringToObject(root, "req_id", req_id);

    cJSON_AddStringToObject(root, "ack", status ? status : "ok");
    if (cmd)
        cJSON_AddStringToObject(root, "cmd", cmd);
    if (extra)
    {
        cJSON_AddItemToObject(root, "data", extra); // move ownership
    }

    char *text = cJSON_PrintUnformatted(root);
    if (text)
    {
        mqtt_publish(reply_topic, text); // mqtt_publish should be non-blocking / qos0/1 as implemented
        free(text);
    }
    else if (extra)
    {
        cJSON_Delete(extra);
    }
    cJSON_Delete(root);
}

// convenience: error ack
static void publish_error(const char *reply_topic, const char *req_id, const char *cmd, const char *reason)
{
    cJSON *extra = cJSON_CreateObject();
    if (extra && reason)
    {
        cJSON_AddStringToObject(extra, "reason", reason);
    }

    publish_ack(reply_topic, "error", cmd, req_id, extra);
}

void commands_init(void)
{
    ESP_LOGI(TAG, "commands init");
}

void commands_deinit(void)
{
    ESP_LOGI(TAG, "commands deinit");
}

// ---------- OTA task helper ----------
typedef struct
{
    char url[256];
    char sha[129]; // 64 byte hex + null  -> 128+1
} ota_params_t;

// Task that actually performs OTA (runs outside MQTT event handler)
static void ota_task(void *arg)
{
    ota_params_t *p = (ota_params_t *)arg;
    if (!p)
    {
        vTaskDelete(NULL);
        return;
    }
    // TODO: ERRROR actuator_shutdown();
    vTaskDelay(pdMS_TO_TICKS(50));
    ESP_LOGI(TAG, "ota_task: starting OTA for %s", p->url);
    ota_start(p->url, p->sha); // blocking: download -> write -> reboot on success

    // ota_start either reboots on success or returns on failure
    // free params and delete task
    free(p);
    vTaskDelete(NULL);
}
// ---------- end OTA helper ----------

void handle_command_json(const char *json, const char *reply_topic)
{
    if (!json || !reply_topic)
        return;

    cJSON *root = cJSON_Parse(json);
    cJSON *req_id = cJSON_GetObjectItem(root, "req_id");
    const char *req_id_str = cJSON_IsString(req_id) ? req_id->valuestring : NULL;

    if (device_busy)
    {
        publish_error(reply_topic, req_id_str, NULL, "device_busy");
        cJSON_Delete(root);
        return;
    }
    if (!root)
    {
        ESP_LOGW(TAG, "invalid json");
        publish_error(reply_topic, req_id_str, NULL, "invalid_json");
        device_busy = false;
        return;
    }

    // 1) LED command
    cJSON *state = cJSON_GetObjectItem(root, "state");
    if (cJSON_IsString(state))
    {
        const char *v = state->valuestring;
        int led_val = 0;
        if (strcmp(v, "on") == 0)
        {
            led_val = 1;
        }
        else if (strcmp(v, "off") == 0)
        {
            led_val = 0;
        }
        else
        {
            publish_error(reply_topic, req_id_str, "led", "invalid_value");
            cJSON_Delete(root);
            device_busy = false;
            return;
        }

        device_busy = true;
        // mqtt_set_led(led_val); TODO : check may be need be removed whole function ,  now we use :state_update_led(led_val);
        state_update_led(led_val);
        cJSON *extra = cJSON_CreateObject();
        cJSON_AddStringToObject(extra, "state", v);
        publish_ack(reply_topic, "ok", "led", req_id_str, extra);
        cJSON_Delete(root);
        device_busy = false;
        return;
    }

    //-------------
    else if (cJSON_GetObjectItem(root, "get_sensors"))
    {
        device_busy = true;
        float t, h, p;
        sensor_read_once(&t, &h, &p); // تو sensor_manager فردا این را اضافه می‌کنیم

        cJSON *extra = cJSON_CreateObject();
        cJSON_AddNumberToObject(extra, "temp", t);
        cJSON_AddNumberToObject(extra, "humidity", h);
        cJSON_AddNumberToObject(extra, "pressure", p);

        publish_ack(reply_topic, "ok", "get_sensors", req_id_str, extra);
        device_busy = false;
        cJSON_Delete(root);
        return;
    }
    //----------------
    // 2) sample_rate
    cJSON *sr = cJSON_GetObjectItem(root, "sample_rate");
    if (cJSON_IsNumber(sr))
    {
        int value = sr->valueint;
        if (value < 100)
        {
            publish_error(reply_topic, req_id_str, "sample_rate", "too_small");
            cJSON_Delete(root);
            device_busy = false;
            return;
        }
        device_busy = true;
        state_update_sample_rate(value);
        cJSON *extra = cJSON_CreateObject();
        cJSON_AddNumberToObject(extra, "sample_rate", value);
        publish_ack(reply_topic, "ok", "sample_rate", req_id_str, extra);
        device_busy = false;
        cJSON_Delete(root);
        return;
    }
    //---------------------
    cJSON *lv = cJSON_GetObjectItem(root, "log_level");
    if (lv)
    {
        if (!cJSON_IsString(lv))
        {
            publish_error(reply_topic, req_id_str, "log_level", "invalid_type");
            cJSON_Delete(root);
            device_busy = false;
            return;
        }

        const char *v = lv->valuestring;
        if (strcmp(v, "debug") == 0)
            dynamic_log_level = ESP_LOG_DEBUG;
        else if (strcmp(v, "info") == 0)
            dynamic_log_level = ESP_LOG_INFO;
        else if (strcmp(v, "warn") == 0)
            dynamic_log_level = ESP_LOG_WARN;
        else if (strcmp(v, "error") == 0)
            dynamic_log_level = ESP_LOG_ERROR;
        else
        {
            publish_error(reply_topic, req_id_str, "log_level", "invalid_value");
            cJSON_Delete(root);
            device_busy = false;
            return;
        }

        // اعمال واقعی سطح لاگ (سراسری یا حداقل برای همین TAG)
        esp_log_level_set("*", dynamic_log_level);
        // یا: esp_log_level_set(TAG, dynamic_log_level);

        cJSON *extra = cJSON_CreateObject();
        cJSON_AddStringToObject(extra, "level", v);
        publish_ack(reply_topic, "ok", "log_level", req_id_str, extra);
        cJSON_Delete(root);
        device_busy = false;
        return;
    }

    //-----------------
    // 3) sys commands
    cJSON *sys = cJSON_GetObjectItem(root, "sys");
    if (cJSON_IsString(sys))
    {
        device_busy = true;
        const char *s = sys->valuestring;
        if (strcmp(s, "reboot") == 0)
        {

            cJSON *extra = cJSON_CreateObject();
            cJSON_AddStringToObject(extra, "action", "rebooting");
            publish_ack(reply_topic, "ok", "reboot", req_id_str, extra);
            vTaskDelay(pdMS_TO_TICKS(200)); // allow ack to go out
            device_busy = false;
            // TODO: ERRROR actuator_shutdown();
            vTaskDelay(pdMS_TO_TICKS(50));
            esp_restart();
            // no return
        }
        else if (strcmp(s, "safe_mode") == 0)
        {
            device_busy = true;
            crash_counter_reset(); // ensure safe
            cJSON *extra = cJSON_CreateObject();
            cJSON_AddStringToObject(extra, "action", "entering_safe_mode");
            publish_ack(reply_topic, "ok", "safe_mode", req_id_str, extra);
            start_safe_mode_task(); // implement in tasks.c
            cJSON_Delete(root);
            device_busy = false;
            return;
        }
        //---------------
        else if (strcmp(s, "decommission") == 0)
        {
            device_busy = true;
            cJSON *extra = cJSON_CreateObject();
            cJSON_AddStringToObject(extra, "action", "decommissioning_credential_wipe");
            publish_ack(reply_topic, "ok", "decommission", req_id_str, extra);
            vTaskDelay(pdMS_TO_TICKS(200));

            // TODO: Should change :        nvs_flash_erase_partition(NVS_PARTITION_CREDENTIALS); // تغییر NVS_PARTITION_CREDENTIALS
            //  می‌توانید بخش‌های دیگری را هم در صورت نیاز اضافه کنید

            device_busy = false;
            cJSON *extra2 = cJSON_CreateObject();
            cJSON_AddStringToObject(extra2, "status", "success");
            publish_ack(reply_topic, "ok", "decommission", req_id_str, extra2);
            cJSON_Delete(extra2);
            return;
        }

        //-------------
        else if (strcmp(s, "factory_reset") == 0)
        {
            device_busy = true;
            cJSON *extra = cJSON_CreateObject();
            cJSON_AddStringToObject(extra, "action", "factory_reset");
            publish_ack(reply_topic, "ok", "factory_reset", req_id_str, extra);
            vTaskDelay(pdMS_TO_TICKS(200));
            nvs_flash_erase();
            device_busy = false;
            esp_restart();
        }

        //-----------
        else if (strcmp(s, "get_fw_info") == 0)
        {
            device_busy = true;
            cJSON *extra = cJSON_CreateObject();
            cJSON_AddStringToObject(extra, "device", "esp32-01");
            cJSON_AddStringToObject(extra, "fw", FW_VERSION);
            // you can add uptime, free heap etc
            cJSON_AddNumberToObject(extra, "free_heap", esp_get_free_heap_size());
            publish_ack(reply_topic, "ok", "info", req_id_str, extra);
            device_busy = false;
            cJSON_Delete(root);
            return;
        }
        else if (strcmp(s, "get_device_info") == 0)
        {
            device_busy = true;
            cJSON *resp = cJSON_CreateObject();

            // device + firmware
            cJSON_AddStringToObject(resp, "device", "esp32-01");
            cJSON_AddStringToObject(resp, "fw", FW_VERSION);

            // uptime (ms)
            cJSON_AddNumberToObject(resp, "uptime_ms", (double)(esp_timer_get_time() / 1000));

            // heap
            cJSON_AddNumberToObject(resp, "heap_free", esp_get_free_heap_size());

            // WiFi RSSI
            wifi_ap_record_t ap;
            if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK)
                cJSON_AddNumberToObject(resp, "wifi_rssi", ap.rssi);
            else
                cJSON_AddNumberToObject(resp, "wifi_rssi", 0);

            // IP address
            esp_netif_ip_info_t ip;
            if (esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip) == ESP_OK)
            {
                char ipbuf[16];
                sprintf(ipbuf, IPSTR, IP2STR(&ip.ip));
                cJSON_AddStringToObject(resp, "ip", ipbuf);
            }
            else
            {
                cJSON_AddStringToObject(resp, "ip", "0.0.0.0");
            }
            // sensor settings
            cJSON_AddNumberToObject(resp, "sample_rate", sensor_get_sample_rate());
            cJSON_AddNumberToObject(resp, "sensor_enabled", sensor_is_enabled());
            // crash counter
            cJSON_AddNumberToObject(resp, "crash_count", crash_counter_get());

            // ACK with full data
            publish_ack(reply_topic, "ok", "get_device_info", req_id_str, resp);
            device_busy = false;
            cJSON_Delete(root);
            return;
        }
        else if (strcmp(s, "get_logs") == 0)
        {
            // TODO: در آینده یک ring buffer لاگ درست می‌کنی
            device_busy = true;
            cJSON *extra = cJSON_CreateObject();
            cJSON_AddStringToObject(extra, "logs", "not_implemented_yet");

            publish_ack(reply_topic, "ok", "get_logs", req_id_str, extra);
            device_busy = false;
            cJSON_Delete(root);
            return;
        }

        else if (strcmp(s, "ota") == 0)
        {
            device_busy = true;
            cJSON *url = cJSON_GetObjectItem(root, "url");
            cJSON *sha = cJSON_GetObjectItem(root, "sha256");

            if (!cJSON_IsString(url) || !cJSON_IsString(sha))
            {
                publish_error(reply_topic, req_id_str, "ota", "missing_url_or_sha");
                cJSON_Delete(root);
                device_busy = false;
                return;
            }

            // ACK immediate
            publish_ack(reply_topic, "ok", "ota", req_id_str, NULL);

            // Allocate params for OTA task
            ota_params_t *p = (ota_params_t *)malloc(sizeof(ota_params_t));
            if (!p)
            {
                publish_error(reply_topic, req_id_str, "ota", "alloc_failed");
                cJSON_Delete(root);
                device_busy = false;
                return;
            }

            // copy safely
            strncpy(p->url, url->valuestring, sizeof(p->url) - 1);
            p->url[sizeof(p->url) - 1] = '\0';
            strncpy(p->sha, sha->valuestring, sizeof(p->sha) - 1);
            p->sha[sizeof(p->sha) - 1] = '\0';

            // Create OTA task (detached). Stack 8192 is safe for HTTP+TLS; priority 5 is fine.
            BaseType_t r = xTaskCreate(ota_task, "ota_task", 8192, p, 5, NULL);
            if (r != pdPASS)
            {
                publish_error(reply_topic, req_id_str, "ota", "task_create_failed");
                free(p);
                cJSON_Delete(root);
                device_busy = false;
                return;
            }

            cJSON_Delete(root);
            device_busy = false;
            return;
        }
    }

    // 4) sensor enable/disable
    cJSON *sensor = cJSON_GetObjectItem(root, "sensor");
    if (cJSON_IsString(sensor))
    {
        device_busy = true;
        cJSON *enable = cJSON_GetObjectItem(root, "enable");
        if (!cJSON_IsNumber(enable))
        {
            publish_error(reply_topic, req_id_str, "sensor", "missing_enable");
            cJSON_Delete(root);
            device_busy = false;
            return;
        }
        const char *name = sensor->valuestring;
        int en = enable->valueint;
        // For now handle only bme280
        if (strcmp(name, "bme280") == 0)
        {
            state_update_sensor_enabled(en);
            cJSON *extra = cJSON_CreateObject();
            cJSON_AddStringToObject(extra, "sensor", name);
            cJSON_AddNumberToObject(extra, "enabled", en);
            publish_ack(reply_topic, "ok", "sensor", req_id_str, extra);
            cJSON_Delete(root);
            device_busy = false;
            return;
        }
        else
        {
            publish_error(reply_topic, req_id_str, "sensor", "unknown_sensor");
            cJSON_Delete(root);
            device_busy = false;
            return;
        }
    }
    else if (cJSON_GetObjectItem(root, "get_info"))
    {
    }

    cJSON *motor = cJSON_GetObjectItem(root, "motor_power");
    if (cJSON_IsNumber(motor))
    {
        int p = motor->valueint;
        if (p < 0 || p > 100)
        {
            ESP_LOGI(TAG, "Wrong pwm Power should 0-100 . power set  : %d", p);
            publish_error(reply_topic, req_id_str, "motor", "invalid_range");
            cJSON_Delete(root);
            return;
        }
        device_busy = true;
        //-------------
        if (actuator_busy)
        {
            publish_error(reply_topic, req_id_str, "motor", "actuator_busy");
            return;
        }
        actuator_busy = true;
        actuator_ramp_to(p);
        actuator_busy = false;
        //-------------
        cJSON *extra = cJSON_CreateObject();
        cJSON_AddNumberToObject(extra, "power", p);
        ESP_LOGI(TAG, "Wrong pwm Power should 0-100 . power set  : %d", p);
        publish_ack(reply_topic, "ok", "motor", req_id_str, extra);

        device_busy = false;
        cJSON_Delete(root);
        return;
    }

    // fallback unknown
    publish_error(reply_topic, req_id_str, NULL, "unknown_command");
    device_busy = false;
    cJSON_Delete(root);
}
