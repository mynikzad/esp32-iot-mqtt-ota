#include "state_manager.h"
#include "config_manager.h"
#include "mqtt_state.h"    
#include "sensor_manager.h" 
#include "driver/gpio.h"  
#include "esp_log.h"
#include "config.h"

static const char *TAG = "STATE_MGR";

// 1. Core State Restore (Reboot Sync)

void state_apply(void)
{
    const device_config_t *cfg = config_get();


    ESP_LOGI(TAG, "Restoring LED state: %d", cfg->led_state);
    gpio_set_level(LED_GPIO, cfg->led_state); 
    ESP_LOGI(TAG, "Restoring Sample Rate: %lu ms", cfg->sample_interval);
    sensor_set_sample_rate(cfg->sample_interval);
    sensor_set_enabled(cfg->sensor_enabled);
    mqtt_state_publish();
}

// 2. Standard State Update from Command

void state_update_led(bool state)
{
    // 1. اعمال به سخت‌افزار
    gpio_set_level(LED_GPIO, state);

    // 2. ذخیره در NVS (فقط در صورت تغییر)
    if (config_get()->led_state != state) {
        config_set_led_state(state);
        config_save(config_get()); // ذخیره پایدار
        ESP_LOGI(TAG, "LED state changed and saved to NVS.");
    }
    
    // 3. Publish (برای Retained و کلاینت‌ها)
    mqtt_state_publish();
}

void state_update_sample_rate(uint32_t rate_ms)
{

    sensor_set_sample_rate(rate_ms); 
    if (config_get()->sample_interval != rate_ms) {
        config_set_sample_interval(rate_ms);
        config_save(config_get());
        ESP_LOGI(TAG, "Sample rate changed and saved to NVS.");
    }
    mqtt_state_publish();
}
void state_update_sensor_enabled(bool enabled)
{
    if (config_get()->sensor_enabled == enabled)
        return;

    // 1) update config
    config_set_sensor_enabled(enabled);
    config_save(config_get());

    // 2) apply side-effect
    sensor_set_enabled(enabled);

    // 3) publish state
    mqtt_state_publish();
}