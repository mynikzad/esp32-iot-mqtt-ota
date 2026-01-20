#pragma once
#include <stdint.h>

void actuator_pwm_init(void);
void actuator_set_power(uint8_t percent);
void actuator_ramp_to(uint8_t target);
void actuator_shutdown(void);
