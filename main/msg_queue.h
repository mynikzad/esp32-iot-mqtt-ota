#pragma once
#include <stdbool.h>
#include <stdlib.h>

typedef enum
{
    MSG_TYPE_SENSOR = 0,
    MSG_TYPE_STATE,
    MSG_TYPE_COMMAND
} msg_type_t;

void msg_queue_init(void);
bool msg_queue_send(const char *json_str, msg_type_t type);
bool msg_queue_receive(char *out, size_t max_len);
bool is_state_message(const char *msg);
