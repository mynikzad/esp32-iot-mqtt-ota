#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <stdbool.h>

void sensor_manager_init(void);            // فراخوانی یکبار در startup
void sensor_manager_start(void);           // ایجاد و استارت تسک نمونه برداری
bool sensor_manager_is_running(void);
void sensor_set_sample_rate(int rate_ms);
void sensor_set_enabled(int enabled);
int sensor_get_sample_rate(void);
int sensor_is_enabled(void);
void sensor_read_once(float *temperature, float *humidity, float *pressure);




#endif // SENSOR_MANAGER_H