#pragma once
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief loads the current config state from NVS and applies it to all hardware
 * This is the core of reboot sync.
 */
void state_apply(void);

/**
 * @brief Applies new LED state, saves to NVS, and publishes
 */
void state_update_led(bool state);

/**
 * @brief Applies new sample rate, saves to NVS, and publishes
 */
void state_update_sample_rate(uint32_t rate_ms);
void state_update_sensor_enabled(bool enabled);