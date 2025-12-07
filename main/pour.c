#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "pca9685.h"
#include "servo_control.h"
#include "http_client.h"  // Your HTTP server functions
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"

#define PCA_FREQ_HZ 50   // Standard servo frequency

static const char *TAG = "MAIN";

void app_main(void)
{
    //Init NVS
    ESP_ERROR_CHECK(nvs_flash_init());

    // Wi-Fi
    ESP_LOGI(TAG, "Connecting to WiFi...");
    ESP_ERROR_CHECK(wifi_init_sta());
    ESP_LOGI(TAG, "WiFi connected!");

    ESP_LOGI(TAG, "Init PCA9685...");
    pca9685_init(50);
    while (1) {
        call_mix_endpoint();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    //Loop forever running remote mix requests
    // pca9685_init(50);
    // while (1) {
    //     for(int i = 4; i < 8; i++) {
    //         extend_nozzle(i, 10000);
    //         vTaskDelay(pdMS_TO_TICKS(1000));
    //     }
    //     //extend_nozzle(4, 2000);
    //     vTaskDelay(pdMS_TO_TICKS(2000));
    // }
    //servo_calibrate(0);
}
