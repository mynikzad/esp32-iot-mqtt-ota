#include "backoff.h"
#include "esp_log.h"

static const char *TAG = "BACKOFF";

// 1, 2, 4, 8, 16, 32 ثانیه (سقف 32 ثانیه)
static int backoff_seconds = 1;

void backoff_reset(void)
{
    backoff_seconds = 1;
}
int backoff_next_delay(void)
{
    int delay = backoff_seconds;

    if (backoff_seconds < 32)
        backoff_seconds *= 2;   // افزایش تا سقف
    ESP_LOGW(TAG, "Backoff delay: %d sec", delay);
    return delay;
}