#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "esp_err.h"

/**
 * @brief Initialize Wi-Fi in STA mode.
 *
 * Connects using the SSID/PASS configured in the implementation file.
 *
 * @return ESP_OK on success, ESP_FAIL on timeout or connection failure.
 */
esp_err_t wifi_init_sta(void);

void extend_nozzle(uint16_t channel, uint32_t ms);

void solenoid_pulse(uint8_t channel, uint32_t ms);

/**
 * @brief Send POST request to remote /mix endpoint.
 *
 * - Sends "{}" as body
 * - Receives JSON like:
 *      {
 *        "status":1,
 *        "recipe":[
 *          {"port":1,"volume_ml":250},
 *          {"port":2,"volume_ml":50}
 *        ]
 *      }
 *
 * - Only proceeds if status == 1
 * - Converts volume_ml → milliseconds
 * - Calls extend_nozzle() for each recipe item
 *
 * @return ESP_OK if successfully processed,
 *         ESP_FAIL on request or JSON parse error.
 */
esp_err_t call_mix_endpoint(void);

#endif /* HTTP_CLIENT_H */
