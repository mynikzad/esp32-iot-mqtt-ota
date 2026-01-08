// Entry point:
// 1. Init WiFi
// 2. Init MQTT
// 3. Create system tasks

//& "C:\Espressif\frameworks\esp-idf-v5.3.1\export.ps1"
#include "wifi.h"
#include "mqtt.h"
#include "tasks.h"
#include "config.h"
#include "sensor_manager.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "crash_counter.h"
#include "esp_event.h"
#include "esp_log.h"
#include "msg_queue.h"
#include "driver/gpio.h"
#include "esp_task_wdt.h"
#include "crash_counter.h"
#include "esp_task_wdt.h"
#include "esp_task_wdt.h"
#include "freertos/task.h"
#include "mqtt_client.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include <inttypes.h>
#include "nvs_flash.h"
#include "utils.h"
#include "config_manager.h"

static const char *TAG_Main = "App Main";

static void on_got_ip(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    // وقتی IP گرفتیم MQTT را start می‌کنیم
    mqtt_start();
}

void app_main(void)
{

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    crash_counter_init();

    //----------------------------------------------------------------------

    config_init(); // NVS init
                   //----------------------TODO: Move to file
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    const device_config_t *cfg = config_get();
    ESP_LOGI("LED", "Restored LED state = %d", cfg->led_state);
    mqtt_set_led(cfg->led_state);
    vTaskDelay(pdMS_TO_TICKS(5000));

    esp_log_level_set("wifi", ESP_LOG_NONE);
    ESP_LOGI(TAG_Main, "Partion----------HERE-----------------");
    const esp_partition_t *p = esp_ota_get_next_update_partition(NULL);
    if (p)
    {
        ESP_LOGW("PART", "Partition info:");
        ESP_LOGW("PART", "  label=%s", p->label);
        ESP_LOGW("PART", "  type=%d", p->type);
        ESP_LOGW("PART", "  subtype=%d", p->subtype);
        ESP_LOGW("PART", "  address=0x%" PRIx32, (uint32_t)p->address);
        ESP_LOGW("PART", "  size=%" PRIu32 " bytes", (uint32_t)p->size);
        ESP_LOGW("PART", "  encrypted=%d", p->encrypted);
    }
    else
    {
        ESP_LOGE("PART", "No OTA partition found!");
    }
    ESP_LOGI(TAG_Main, "Partion----------End-----------------");
    // vTaskDelay(pdMS_TO_TICKS(5000));

    //-------------------------

    esp_task_wdt_deinit();
    esp_task_wdt_config_t wdt_cfg = {
        .timeout_ms = 300000, // 300s
        .trigger_panic = true,
    };

    esp_err_t err = esp_task_wdt_init(&wdt_cfg);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGI(TAG_Main, "esp_task_wdt_init failed: %s", esp_err_to_name(err));
    }

    //----------------------------------------------------------------

    // Increase crash count (device just rebooted)
    crash_counter_increment();
    // Check Safe Mode
    if (crash_counter_is_safe_mode())
    {
        ESP_LOGE("SAFE_MODE", "Device entered SAFE MODE due to crash loop!");
        // start only LED blink task
        start_safe_mode_task();
        return; // do NOT start normal system
    }
    ESP_LOGI(TAG_Main, FW_VERSION);

    const device_config_t *cfgA = config_get();
    ESP_LOGI("TEST", "SSID = %s", cfgA->wifi_ssid);
    ESP_LOGI("TEST", "PASS = %s", cfgA->wifi_pass);
    vTaskDelay(pdMS_TO_TICKS(7000));

    wifi_init();
    wifi_start();
    mqtt_init();
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_got_ip, NULL, NULL);
    sensor_manager_init();
    sensor_manager_start();
    msg_queue_init();
    create_tasks();
    esp_ota_mark_app_valid_cancel_rollback();
    log_filter_init();
}
