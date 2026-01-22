#include "sensor_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "cJSON.h"
#include "mqtt.h" // تابع mqtt_publish_sensor_data یا mqtt_publish_status
#include "config.h"
#include <math.h>
#include <stdlib.h>
#include "msg_queue.h"
#include "esp_task_wdt.h"

#include "sensor_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static int sensor_enabled = 1;
static int sample_rate_ms = 150000;
static const char *TAG = "SENSOR_MGR";

// هر 500 ms نمونه می‌گیریم (2 Hz)
#define DEVICE_ID "esp32-01"

static TaskHandle_t sensor_task_handle = NULL;
static bool running = false;

/* ---- شبیه‌ساز ساده: پنج بار افزایش، پنج بار کاهش ----
   pattern_len = 10, step = amplitude / 5 */
static void sensor_task(void *pv)
{
    if (esp_task_wdt_add(NULL) != ESP_OK)
    {
        ESP_LOGE(TAG, "WDT add failed for Sensor Manager");
    }

    int idx = 0;
    const float temp_base = 22.0f;
    const float temp_amp = 3.0f;

    const float hum_base = 50.0f;
    const float hum_amp = 10.0f;
    const int hum_period_steps = 10; // 10 نمونه یک دوره (پله بالا/پایین)

    const float pres_base = 1013.0f;
    const float pres_noise = 1.5f;

    while (1)
    {
        esp_task_wdt_reset();
        if (!sensor_enabled)
        {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        // زمان به ثانیه
        double t = (double)esp_timer_get_time() / 1000000.0;

        // temp: سینوسی آهسته
        float temp = temp_base + temp_amp * sinf(2.0f * M_PI * 0.1f * (float)t);

        // humidity: پله‌ای بالا/پایین با دوره hum_period_steps
        int phase = idx % hum_period_steps;
        float hum;
        if (phase < hum_period_steps / 2)
            hum = hum_base + ((float)phase / (hum_period_steps / 2 - 1.0f)) * hum_amp;
        else
            hum = hum_base + ((float)(hum_period_steps - 1 - phase) / (hum_period_steps / 2 - 1.0f)) * hum_amp;

        // pressure: پایه + نویز رندوم
        float pres = pres_base + (((float)rand() / (float)RAND_MAX) - 0.5f) * 2.0f * pres_noise;

        // timestamp (ms)
        int64_t ts_ms = esp_timer_get_time() / 1000;

        // ساخت JSON
        cJSON *root = cJSON_CreateObject();
        if (root)
        {
            cJSON_AddStringToObject(root, "device", DEVICE_ID);
            cJSON_AddStringToObject(root, "fw", FW_VERSION);
            cJSON_AddNumberToObject(root, "timestamp_ms", (double)ts_ms);

            cJSON *sobj = cJSON_CreateObject();
            if (sobj)
            {
                cJSON_AddNumberToObject(sobj, "temp", temp);
                cJSON_AddNumberToObject(sobj, "humidity", hum);
                cJSON_AddNumberToObject(sobj, "pressure", pres);
                cJSON_AddItemToObject(root, "sensors", sobj);
            }

            char *json_str = cJSON_PrintUnformatted(root);
            if (json_str)
            {
                // Forward sensor data to publishing task via queue (non-blocking)

                msg_queue_send(json_str, MSG_TYPE_SENSOR);
                ESP_LOGI(TAG, "Published: %s", json_str);
                cJSON_free(json_str);
            }
            cJSON_Delete(root);
        }

        idx++;
        vTaskDelay(pdMS_TO_TICKS(sample_rate_ms));
    }
}

void sensor_manager_init(void)
{
    running = false;
    srand((unsigned)esp_timer_get_time()); // seed ساده برای نویز
}

void sensor_manager_start(void)
{
    if (sensor_task_handle != NULL)
        return;
    BaseType_t r = xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, &sensor_task_handle);
    if (r == pdPASS)
    {
        running = true;
        ESP_LOGI(TAG, "sensor task started");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to create sensor task");
    }
}

bool sensor_manager_is_running(void)
{
    return running;
}
// مقدار پیش‌فرض

void sensor_set_sample_rate(int rate_ms)
{
    if (rate_ms < 500)
        rate_ms = 500;
    sample_rate_ms = rate_ms;
    ESP_LOGI(TAG, "sample rate set to %d ms", sample_rate_ms);
}
void sensor_set_enabled(int enabled)
{
    sensor_enabled = enabled ? 1 : 0;
    if (sensor_enabled && !sensor_manager_is_running())
        sensor_manager_start();
    ESP_LOGI(TAG, "sensor enabled = %d", sensor_enabled);
}
int sensor_get_sample_rate(void)
{
    return sample_rate_ms;
}

int sensor_is_enabled(void)
{
    return sensor_enabled;
}
void sensor_read_once(float *temperature, float *humidity, float *pressure)
{
    double t = (double)esp_timer_get_time() / 1000000.0;
    *temperature = 22.0f + 3.0f * sinf(2.0f * M_PI * 0.1f * (float)t);
    *humidity = 50.0f + 10.0f * sinf(2.0f * M_PI * 0.05f * (float)t);
    *pressure = 1013.0f + (((float)rand() / (float)RAND_MAX) - 0.5f) * 3.0f;
}
