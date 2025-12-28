#ifndef CRASH_COUNTER_H
#define CRASH_COUNTER_H

#include <stdbool.h>

void safe_mode_task(void *pv);
void crash_counter_init(void);
void crash_counter_increment(void);
void crash_counter_reset(void);
bool crash_counter_is_safe_mode(void);
void start_safe_mode_task(void);
int crash_counter_get(void);
#endif
