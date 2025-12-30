// OTA is triggered by MQTT command
#include "ota.h"
#include "esp_ota_ops.h"

#include "esp_http_client.h"
#include <esp_system.h>

#include "esp_log.h"
#include "mbedtls/sha256.h"
#include <string.h>
#include <ctype.h>
static const char *TAG = "OTA";

static ota_state_t ota_state = OTA_IDLE;
static char last_error[128] = {0};

extern const char cert_pem_start[] asm("_binary_cert_pem_start");
extern const char cert_pem_end[] asm("_binary_cert_pem_end");

static void set_error(const char *msg)
{
    ota_state = OTA_FAILED;
    strncpy(last_error, msg, sizeof(last_error) - 1);
    ESP_LOGE(TAG, "%s", msg);
}

const char *ota_get_last_error(void)
{
    return last_error;
}

ota_state_t ota_get_state(void)
{
    return ota_state;
}

void ota_start(const char *url, const char *expected_sha256_hex)
{
    ota_state = OTA_DOWNLOADING;
    ESP_LOGW(TAG, "OTA START: %s", url);

    esp_http_client_config_t config = {
        .url = url,
        .cert_pem = cert_pem_start, // اگر HTTPS → اینجا CA cert اضافه می‌شود
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client)
    {
        set_error("http_client_init_failed");
        return;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK)
    {
        set_error("http_open_failed");
        return;
    }

    int content_length = esp_http_client_fetch_headers(client);
    if (content_length <= 0)
    {
        set_error("invalid_content_length");
        return;
    }

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    esp_ota_handle_t ota_handle;

    err = esp_ota_begin(update_partition, content_length, &ota_handle);
    if (err != ESP_OK)
    {
        set_error("ota_begin_failed");
        return;
    }

    uint8_t buf[4096];
    mbedtls_sha256_context sha;
    mbedtls_sha256_init(&sha);
    mbedtls_sha256_starts(&sha, 0);

    int total = 0;

    while (1)
    {
        int r = esp_http_client_read(client, (char *)buf, sizeof(buf));
        if (r < 0)
        {
            set_error("http_read_failed");
            esp_ota_end(ota_handle);
            return;
        }
        if (r == 0)
            break;

        mbedtls_sha256_update(&sha, buf, r);

        err = esp_ota_write(ota_handle, buf, r);
        if (err != ESP_OK)
        {
            set_error("ota_write_failed");
            esp_ota_end(ota_handle);
            return;
        }

        total += r;
        ESP_LOGI(TAG, "Downloaded %d / %d bytes", total, content_length);
    }

    uint8_t sha256_actual[32];
    mbedtls_sha256_finish(&sha, sha256_actual);

    char sha_hex[65];
    for (int i = 0; i < 32; i++)
        sprintf(sha_hex + i * 2, "%02x", sha256_actual[i]);

    while (*expected_sha256_hex && isspace((unsigned char)*expected_sha256_hex))
        expected_sha256_hex++;
    // case-insensitive compare
    ESP_LOGI(TAG, "expected sha: %s", expected_sha256_hex);
    ESP_LOGI(TAG, "actual   sha: %s", sha_hex);
    if (strcasecmp(sha_hex, expected_sha256_hex) != 0)
    {
        set_error("sha256_mismatch");
        esp_ota_end(ota_handle);
        return;
    }

    if ((err = esp_ota_end(ota_handle)) != ESP_OK)
    {
        set_error("ota_end_failed");
        return;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK)
    {
        set_error("set_boot_partition_failed");
        return;
    }

    ota_state = OTA_SUCCESS;
    ESP_LOGW(TAG, "OTA SUCCESS → REBOOT");
    esp_restart();
}
