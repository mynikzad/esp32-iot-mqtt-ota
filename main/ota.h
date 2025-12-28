#pragma once
#include "esp_err.h"

typedef enum {
    OTA_IDLE = 0,
    OTA_DOWNLOADING,
    OTA_FLASHING,
    OTA_SUCCESS,
    OTA_FAILED
} ota_state_t;

void ota_start(const char *url, const char *expected_sha256);
ota_state_t ota_get_state(void);
const char* ota_get_last_error(void);
