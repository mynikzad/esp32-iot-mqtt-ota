#include "msg_queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include <string.h>

#define QUEUE_MSG_MAX_LEN   512
#define QUEUE_LENGTH        20

static const char *TAG = "MSG_QUEUE";
static QueueHandle_t msg_queue = NULL;

void msg_queue_init(void)
{
    msg_queue = xQueueCreate(QUEUE_LENGTH, QUEUE_MSG_MAX_LEN);
    if (!msg_queue) {
        ESP_LOGE(TAG, "Failed to create queue");
    } else {
        ESP_LOGI(TAG, "Queue created");
    }
}

bool msg_queue_send(const char *json_str)
{
    if (!msg_queue) return false;
    char buffer[QUEUE_MSG_MAX_LEN];
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, json_str, sizeof(buffer) - 1);
    return xQueueSend(msg_queue, buffer, 0) == pdTRUE;
}

bool msg_queue_receive(char *out, size_t max_len)
{
    if (!msg_queue) return false;
    return xQueueReceive(msg_queue, out, pdMS_TO_TICKS(1000)) == pdTRUE;
}
