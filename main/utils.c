#include "utils.h"
#include "esp_log.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
// TODO: Here should be change -- just for test now
static int custom_log_filter(const char *fmt, va_list args)
{
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    // Remove some Special LOG
    if (strstr(buffer, "transport_base: Failed to open a new connection") != NULL)
    {
        return 0;
    }
    else if (strstr(buffer, "MQTT: MQTT generic error") != NULL)
    {
        return 0;
    }
    return vprintf(fmt, args);
}

void log_filter_init(void)
{
    esp_log_set_vprintf(custom_log_filter);
}
