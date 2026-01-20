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
#include "esp_ota_ops.h"
#include "MyTest.h"

// static const char *TAG1 = "TASKS";
static const char *TAG_TASK = "TASKS";

void heartbeat_task(void *pv)
{
    if (esp_task_wdt_add(NULL) != ESP_OK)
    {
        ESP_LOGW(TAG_TASK, "WDT add failed for heartbeat_task");
    }
    while (1)
    {

        esp_task_wdt_reset(); // ← مهم
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
    if (esp_task_wdt_add(NULL) != ESP_OK)
    {
        ESP_LOGW("SAFE_MODE", "WDT add failed for safe_mode_task");
    }
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT); // LED
    actuator_shutdown();
    vTaskDelay(pdMS_TO_TICKS(50));
    //TODO : ERROR sensor_set_enabled(0);

    while (1)
    {
        esp_task_wdt_reset();
        gpio_set_level(GPIO_NUM_2, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level(GPIO_NUM_2, 0);
        vTaskDelay(pdMS_TO_TICKS(800));
    }
}

void wifi_reconnect_task(void *pv)
{
    if (esp_task_wdt_add(NULL) != ESP_OK)
    {
        ESP_LOGW(TAG_TASK, "WDT add failed for wifi_reconnect_task");
    }
    int delay_sec = 1;

    while (!wifi_has_initialized())
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    while (1)
    {
        esp_task_wdt_reset(); // ← مهم
        if (!wifi_is_connected())
        {
            ESP_LOGW(TAG_TASK, "WiFi lost → reconnecting...");
            esp_wifi_connect();

            delay_sec = backoff_next_delay();
            vTaskDelay(pdMS_TO_TICKS(delay_sec * 1000));
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}

void mqtt_reconnect_task(void *pv)
{

    if (esp_task_wdt_add(NULL) != ESP_OK)
    {
        ESP_LOGW(TAG_TASK, "WDT add failed for mqtt_reconnect_task");
    }
    int delay_sec = 1;
    while (1)
    {
        esp_task_wdt_reset(); // ← مهم
        if (!mqtt_is_connected())
        {
            ESP_LOGW(TAG_TASK, "MQTT lost → reconnecting...");
            mqtt_try_reconnect();

            delay_sec = backoff_next_delay();
            vTaskDelay(pdMS_TO_TICKS(delay_sec * 1000));
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(3000));
        }
    }
}

void mqtt_queue_sender_task(void *pv)
{
    char buffer[512];
    if (esp_task_wdt_add(NULL) != ESP_OK)
    {
        ESP_LOGW(TAG_TASK, "WDT add failed for mqtt_queue_sender_task");
    }
    while (1)
    {
        esp_task_wdt_reset(); // ← مهم
        if (mqtt_is_connected())
        {
            if (msg_queue_receive(buffer, sizeof(buffer)))
            {
                mqtt_publish_sensor_data(buffer);
                ESP_LOGI("QUEUE_SENDER", "MQTT published: %s", buffer);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
//-----------------------------Create for check system Live
void CheckDeviceVolt(void *pv)
{

    if (esp_task_wdt_add(NULL) != ESP_OK)
    {
        ESP_LOGW(TAG_TASK, "WDT add failed for Device Check Volt");
    }
    while (1)
    {
        esp_task_wdt_reset(); // ← مهم
      /* TODO : ERROR  if (power_get_voltage() < 3.0)
        {
            system_enter_fail_safe();
        }*/
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
//-----------------------------
// TODO: its not good enaph i should fix it late
void rollback_watchdog_task(void *pvParameters)
{
    ESP_LOGI(TAG_TASK, "----Here Task Set Just one ----");
    const TickType_t timeout = 75000 / portTICK_PERIOD_MS;
    vTaskDelay(timeout);
    ESP_LOGI(TAG_TASK, "----Here To Start Roll back ----");
    esp_ota_mark_app_invalid_rollback_and_reboot();
    vTaskDelete(NULL);
}

void pwm_visual_test_task(void *pvParameters)
{
     // ⬅️ اضافه کردن این تسک به WDT سیستمی
    if (esp_task_wdt_add(NULL) != ESP_OK)
    {
        ESP_LOGW(TAG_TASK, "WDT add failed for pwm_visual_test_task");
    }
    
    ESP_LOGI("PWM_VISUAL_TEST", "Starting visual PWM ramp test.");

    while (1)
    {
        esp_task_wdt_reset(); // 
        // تست افزایش از 0 تا 100
        actuator_ramp_to(100);
        vTaskDelay(pdMS_TO_TICKS(1000)); // توقف در اوج به مدت 1 ثانیه

        // تست کاهش از 100 تا 0
        actuator_ramp_to(0);
        vTaskDelay(pdMS_TO_TICKS(1000)); // توقف در حداقل به مدت 1 ثانیه
        
        // برای تکرار سریع‌تر، می‌توانید این دیلی را حذف یا کم کنید.
        // vTaskDelay(pdMS_TO_TICKS(500)); 
    }

    // اگر از حلقه خارج شد (که نباید شود)، تسک را پاک کنید.
    //vTaskDelete(NULL);
}


void create_tasks(void)
{
    xTaskCreate(heartbeat_task, "hb", 4096, NULL, 5, NULL);
    xTaskCreate(wifi_reconnect_task, "w_recon", 4096, NULL, 5, NULL);
    xTaskCreate(mqtt_reconnect_task, "m_recon", 4096, NULL, 5, NULL);
    xTaskCreate(mqtt_queue_sender_task, "q_sender", 4096, NULL, 5, NULL);
    xTaskCreate(rollback_watchdog_task, "rollback_wd", 4096, NULL, 5, NULL);
     xTaskCreatePinnedToCore(pwm_visual_test_task, "pwm_visual_test", 4096, NULL, 5, NULL, 1);
    xTaskCreate(CheckDeviceVolt, "CheckVoltage", 2048, NULL, 5, NULL);
}
