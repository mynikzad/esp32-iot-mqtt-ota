// crash_counter.c
#include "crash_counter.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <stdint.h>
#include <inttypes.h>

static const char *TAG = "CRASH_COUNTER";

#define NVS_NAMESPACE "crash"
#define KEY_COUNT "count"
#define KEY_TIME  "time"

#define MAX_CRASHES 5
#define TIME_WINDOW_US (2 * 60 * 1000 * 1000ULL)   // 2 دقیقه

static int32_t crash_count = 0;
static uint64_t last_crash_time = 0;

void crash_counter_init(void)
{
    esp_err_t err;
    nvs_handle_t nvs;
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(err));
        return;
    }

    // get stored count (if exists)
    int32_t stored_count = 0;
    err = nvs_get_i32(nvs, KEY_COUNT, &stored_count);
    if (err == ESP_OK) {
        crash_count = stored_count;
    } else if (err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "nvs_get_i32 failed: %s", esp_err_to_name(err));
    }

    // get stored time (if exists)
    uint64_t stored_time = 0;
    err = nvs_get_u64(nvs, KEY_TIME, &stored_time);
    if (err == ESP_OK) {
        last_crash_time = stored_time;
    } else if (err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "nvs_get_u64 failed: %s", esp_err_to_name(err));
    }

    nvs_close(nvs);
}

void crash_counter_increment(void)
{
    uint64_t now = esp_timer_get_time();  // microseconds

    if (now - last_crash_time > TIME_WINDOW_US) {
        crash_count = 0;
    }

    crash_count++;
    last_crash_time = now;

    nvs_handle_t nvs;
    nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs);
    nvs_set_i32(nvs, KEY_COUNT, crash_count);
    nvs_set_u64(nvs, KEY_TIME, last_crash_time);
    nvs_commit(nvs);
    nvs_close(nvs);

    ESP_LOGW(TAG, "Crash count updated: %"PRId32, crash_count);
}

void crash_counter_reset(void)
{
    crash_count = 0;

    nvs_handle_t nvs;
    nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs);
    nvs_set_i32(nvs, KEY_COUNT, 0);
    nvs_commit(nvs);
    nvs_close(nvs);

    ESP_LOGI(TAG, "Crash counter reset");
}

bool crash_counter_is_safe_mode(void)
{
    return (crash_count >= MAX_CRASHES);
}
int crash_counter_get(void)
{
    return crash_count;  // همان متغیر داخلی که داریم می‌شماریم
}