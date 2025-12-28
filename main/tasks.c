#include "tasks.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt.h"
#include "wifi.h"
#include "esp_wifi.h"
#include "msg_queue.h"
#include "backoff.h"
#include "esp_task_wdt.h"
#include "driver/gpio.h"
#include "crash_counter.h"


// static const char *TAG1 = "TASKS";
static const char *TAG_TASK = "TASKS";

void heartbeat_task(void *pv)
{
    if (esp_task_wdt_add(NULL) != ESP_OK) {
        ESP_LOGW(TAG_TASK, "WDT add failed for heartbeat_task");
    }
    while (1)
    {
        
        esp_task_wdt_reset();  // ← مهم
        if (mqtt_is_connected())
        {
            mqtt_publish_status("alive");
            ESP_LOGI(TAG_TASK, "heartbeat published");
        }
        vTaskDelay(pdMS_TO_TICKS(180000));
    }
}
void start_safe_mode_task(void)
{
    xTaskCreate(safe_mode_task, "SAFE_MODE", 2048, NULL, 10, NULL);
}
void safe_mode_task(void *pv)
{
     if (esp_task_wdt_add(NULL) != ESP_OK) {
        ESP_LOGW("SAFE_MODE", "WDT add failed for safe_mode_task");
    }
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);  // LED

    while (1) {
         esp_task_wdt_reset(); 
        gpio_set_level(GPIO_NUM_2, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level(GPIO_NUM_2, 0);
        vTaskDelay(pdMS_TO_TICKS(800));
    }
}

void wifi_reconnect_task(void *pv)
{
     if (esp_task_wdt_add(NULL) != ESP_OK) {
        ESP_LOGW(TAG_TASK, "WDT add failed for wifi_reconnect_task");
    }
    int delay_sec = 1;

    while (!wifi_has_initialized()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    while (1) {
        esp_task_wdt_reset();  // ← مهم
        if (!wifi_is_connected()) {
            ESP_LOGW(TAG_TASK, "WiFi lost → reconnecting...");
            esp_wifi_connect();

            delay_sec = backoff_next_delay();
            vTaskDelay(pdMS_TO_TICKS(delay_sec * 1000));
        } else {
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}


void mqtt_reconnect_task(void *pv)
{
    
 if (esp_task_wdt_add(NULL) != ESP_OK) {
        ESP_LOGW(TAG_TASK, "WDT add failed for mqtt_reconnect_task");
    }
    int delay_sec = 1;
    while (1) {
esp_task_wdt_reset();  // ← مهم
        if (!mqtt_is_connected()) {
            ESP_LOGW(TAG_TASK, "MQTT lost → reconnecting...");
            mqtt_try_reconnect();

            delay_sec = backoff_next_delay();
            vTaskDelay(pdMS_TO_TICKS(delay_sec * 1000));
        } else {
            vTaskDelay(pdMS_TO_TICKS(3000));
        }
    }
}


void mqtt_queue_sender_task(void *pv)
{
    char buffer[512];
if (esp_task_wdt_add(NULL) != ESP_OK) {
        ESP_LOGW(TAG_TASK, "WDT add failed for mqtt_queue_sender_task");
    }
    while (1) {
       esp_task_wdt_reset();  // ← مهم
        if (mqtt_is_connected()) {
            if (msg_queue_receive(buffer, sizeof(buffer))) {
                mqtt_publish_sensor_data(buffer);
                ESP_LOGI("QUEUE_SENDER", "MQTT published: %s", buffer);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


void create_tasks(void)
{
    xTaskCreate(heartbeat_task, "hb", 4096, NULL, 5, NULL);
    xTaskCreate(wifi_reconnect_task, "w_recon", 4096, NULL, 5, NULL);
    xTaskCreate(mqtt_reconnect_task, "m_recon", 4096, NULL, 5, NULL);
    xTaskCreate(mqtt_queue_sender_task, "q_sender", 4096, NULL, 5, NULL);

}

