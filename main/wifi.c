#include "wifi.h"
#include "config.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "backoff.h"
static volatile bool wifi_initialized = false;
static volatile bool wifi_connected = false;
static const char *TAG_wifi = "WIFI";
static volatile bool wifi_first_connect_done = false;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_connected = false;
        ESP_LOGE(TAG_wifi, "WiFi disconnected (reason=%d)", ((wifi_event_sta_disconnected_t*)event_data)->reason);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        wifi_first_connect_done = true;
        wifi_connected = true;
        ESP_LOGI(TAG_wifi, "WiFi connected, IP received");
    }
    // توجه: این تابع دیگر نباید مستقیماً mqtt_start() را اجرا کند.
    // به جای آن در app_main یا task مناسب فراخوانی می‌کنیم.
}

void wifi_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

void wifi_start(void)
{
    wifi_config_t wifi_config =  {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS}};
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));
    
    
    wifi_initialized = true;
    esp_wifi_connect();
}

bool wifi_is_connected(void)
{
    return wifi_connected;
}
bool wifi_has_initialized(void)
{
    return wifi_initialized;
}