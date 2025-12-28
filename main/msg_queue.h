#pragma once
#include <stdbool.h>
#include <stdlib.h>

void msg_queue_init(void);
bool msg_queue_send(const char *json_str);
bool msg_queue_receive(char *out, size_t max_len);
