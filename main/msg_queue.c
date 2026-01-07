// Message queue used to decouple MQTT commands from system tasks
#include "msg_queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include <string.h>

#define QUEUE_MSG_MAX_LEN 512
#define QUEUE_LENGTH 20

static const char *TAG = "MSG_QUEUE";
static QueueHandle_t msg_queue = NULL;

void msg_queue_init(void)
{
    msg_queue = xQueueCreate(QUEUE_LENGTH, QUEUE_MSG_MAX_LEN);
    if (!msg_queue)
    {
        ESP_LOGE(TAG, "Failed to create queue");
    }
    else
    {
        ESP_LOGI(TAG, "Queue created");
    }
}

/*bool msg_queue_send(const char *json_str)
{
    if (!msg_queue)
        return false;
    char buffer[QUEUE_MSG_MAX_LEN];
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, json_str, sizeof(buffer) - 1);
    return xQueueSend(msg_queue, buffer, 0) == pdTRUE;
}*/
bool msg_queue_send(const char *json_str, msg_type_t type)
{
    if (!msg_queue)
    {
        ESP_LOGE(TAG, "Queue not initialized");
        return false;
    }

    char buffer[QUEUE_MSG_MAX_LEN];
    strlcpy(buffer, json_str, sizeof(buffer));

    // Try to send with timeout
    if (xQueueSend(msg_queue, buffer, 0) != pdTRUE)
    {
        ESP_LOGE(TAG, "Queue insert failed after drop");
        return false;
    }
    return true;

    // Queue is full → handle overflow
    ESP_LOGW(TAG, "Queue full, applying overflow policy");

    char dropped[QUEUE_MSG_MAX_LEN];

    if (type == MSG_TYPE_COMMAND)
    {
        // Drop first STATE message
        if (xQueuePeek(msg_queue, dropped, 0) == pdTRUE &&
            is_state_message(dropped))
        {
            xQueueReceive(msg_queue, dropped, 0);
            ESP_LOGW(TAG, "Dropped oldest STATE due to COMMAND");
        }
        else
        {
            // Otherwise drop oldest anyway
            xQueueReceive(msg_queue, dropped, 0);
        }
    }
    else
    {
        // Normal message → drop oldest
        xQueueReceive(msg_queue, dropped, 0);
    }

    // Insert new message
    xQueueSend(msg_queue, buffer, 0);
    return true;
}

bool msg_queue_receive(char *out, size_t max_len)
{
    if (!msg_queue)
        return false;
    return xQueueReceive(msg_queue, out, pdMS_TO_TICKS(1000)) == pdTRUE;
}
bool is_state_message(const char *msg)
{
    return (strstr(msg, "\"state\"") != NULL);
}
