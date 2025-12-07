#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "esp_http_client.h"

#include "pca9685.h"
#include "servo_control.h"

#define WIFI_SSID "DukeVisitor"
#define WIFI_PASS ""

static const char *TAG = "MAIN";

/* ---------------------- Servo Logic ---------------------- */

void solenoid_pulse(uint8_t channel, uint32_t ms)
{
    pca9685_set_duty(channel, 100);
    vTaskDelay(pdMS_TO_TICKS(ms));
    pca9685_set_duty(channel, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
}

void extend_nozzle(uint16_t channel, uint32_t ms)
{
    servo_rotate_cw(channel, 0.9f);
    solenoid_pulse(channel + 8, ms);
    servo_rotate_ccw(channel, 0.75f);
}

/* ---------------------- Wi-Fi ---------------------- */

static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();

    } else if (event_base == WIFI_EVENT &&
               event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi disconnected, retrying...");
        esp_wifi_connect();

    } else if (event_base == IP_EVENT &&
               event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                   &wifi_event_handler, NULL)
    );
    ESP_ERROR_CHECK(
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                   &wifi_event_handler, NULL)
    );

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_OPEN,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group, WIFI_CONNECTED_BIT,
        pdFALSE, pdFALSE, pdMS_TO_TICKS(15000)
    );

    return (bits & WIFI_CONNECTED_BIT) ? ESP_OK : ESP_FAIL;
}

/* ---------------------- HTTP Client ---------------------- */
#ifndef TAG
#define TAG "MAIN"
#endif
 
typedef struct {
    char *buffer;
    int   buffer_len;
    int   data_len;
} http_resp_ctx_t;
 
/**
 * HTTP event handler: accumulate response body into user_data buffer.
 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    http_resp_ctx_t *ctx = (http_resp_ctx_t *)evt->user_data;
 
    switch (evt->event_id) {
    case HTTP_EVENT_ON_DATA:
        // For non-chunked responses, just copy into our buffer
        if (!esp_http_client_is_chunked_response(evt->client)) {
            if (ctx && ctx->buffer) {
                int copy_len = evt->data_len;
 
                // Clamp to remaining space (leave room for '\0')
                if (ctx->data_len + copy_len >= ctx->buffer_len) {
                    copy_len = ctx->buffer_len - 1 - ctx->data_len;
                }
 
                if (copy_len > 0) {
                    memcpy(ctx->buffer + ctx->data_len, evt->data, copy_len);
                    ctx->data_len += copy_len;
                }
            }
        }
        break;
 
    case HTTP_EVENT_ON_FINISH:
        if (ctx && ctx->buffer && ctx->data_len < ctx->buffer_len) {
            ctx->buffer[ctx->data_len] = '\0';  // null-terminate
        }
        break;
 
    default:
        break;
    }
 
    return ESP_OK;
}
 
/**
 * Call /mix endpoint, parse JSON, and run extend_nozzle() on recipe when status==1.
 */
esp_err_t call_mix_endpoint(void)
{
    const char *url       = "http://3.140.199.217:8081/mix";   // same as your curl, but with :8081
    const char *post_body = "{}";
 
    static char resp_buffer[256];  // plenty for {"status":2} or a small recipe
    http_resp_ctx_t resp_ctx = {
        .buffer     = resp_buffer,
        .buffer_len = sizeof(resp_buffer),
        .data_len   = 0,
    };
 
    esp_http_client_config_t cfg = {
        .url           = url,
        .method        = HTTP_METHOD_POST,
        .timeout_ms    = 100000,
        .event_handler = http_event_handler,
        .user_data     = &resp_ctx,
    };
 
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        return ESP_FAIL;
    }
 
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_body, strlen(post_body));
 
    ESP_LOGI(TAG, "Calling /mix endpoint...");
 
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "POST failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }
 
    int status_code = esp_http_client_get_status_code(client);
    int content_len = esp_http_client_get_content_length(client);
    ESP_LOGI(TAG, "HTTP status=%d, content_len=%d", status_code, content_len);
 
    esp_http_client_cleanup(client);
 
    // Body should now be in resp_ctx.buffer (filled by http_event_handler)
    ESP_LOGI(TAG, "Response (len=%d): '%s'",
             resp_ctx.data_len,
             (resp_ctx.data_len > 0) ? resp_ctx.buffer : "");
 
    if (resp_ctx.data_len == 0) {
        ESP_LOGE(TAG, "Empty response body, cannot parse JSON");
        return ESP_FAIL;
    }
 
    // ---- Parse JSON ----
    cJSON *root = cJSON_Parse(resp_ctx.buffer);
    if (!root) {
        ESP_LOGE(TAG, "JSON parse failed");
        return ESP_FAIL;
    }
 
    cJSON *status = cJSON_GetObjectItem(root, "status");
    if (!cJSON_IsNumber(status)) {
        ESP_LOGE(TAG, "No numeric 'status' in response");
        cJSON_Delete(root);
        return ESP_FAIL;
    }
 
    ESP_LOGI(TAG, "Parsed status=%d", status->valueint);
 
    // Only mix when status == 1 (your logic)
    if (status->valueint != 1) {
        ESP_LOGW(TAG, "Skipping: status != 1 (got %d)", status->valueint);
        cJSON_Delete(root);
        return ESP_OK;
    }
 
    cJSON *recipe = cJSON_GetObjectItem(root, "recipe");
    if (!cJSON_IsArray(recipe)) {
        ESP_LOGE(TAG, "Missing 'recipe'");
        cJSON_Delete(root);\
        return ESP_FAIL;
    }
 
    int count = cJSON_GetArraySize(recipe);
    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(recipe, i);
        if (!cJSON_IsObject(item)) {
            ESP_LOGW(TAG, "Skipping non-object item in recipe");
            continue;
        }
 
        cJSON *port_json   = cJSON_GetObjectItem(item, "port");
        cJSON *volume_json = cJSON_GetObjectItem(item, "volume_ml");
        if (!cJSON_IsNumber(port_json) || !cJSON_IsNumber(volume_json)) {
            ESP_LOGW(TAG, "Invalid recipe item (missing port/volume_ml)");
            continue;
        }
 
        int port      = port_json->valueint;
        int volume_ml = volume_json->valueint;
        ESP_LOGI(TAG, "Recipe item: port=%d, volume_ml=%d", port, volume_ml);
 
        extend_nozzle(port, (volume_ml*20/50)*1000);
    }
 
    cJSON_Delete(root);
    return ESP_OK;
}