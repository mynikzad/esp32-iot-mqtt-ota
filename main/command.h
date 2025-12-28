#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdbool.h>

void commands_init(void);
void handle_command_json(const char *json, const char *reply_topic);
void commands_deinit(void);

#endif // COMMANDS_H
