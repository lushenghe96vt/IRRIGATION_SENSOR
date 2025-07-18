#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "sensor.h"
#include "wifi_manager.h"

#define TAG "SENSOR_NODE"
// controller IP
#define CONTROLLER_URL "http://192.168.4.1/moisture"
// GPIO pins for output: 2, 4, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27
#define MOISTURE_OUT_D4 4
// GPIO pins for ADC input: 32 ADC1_CHANNEL_4, 33 ADC1_CHANNEL_5, 34 ADC1_CHANNEL_6, 35 ADC1_CHANNEL_7
#define MOISTURE_IN_D32 ADC1_CHANNEL_4

// helper function for post
void post_moisture_data(MoistureSensor *sensor) {
    const char *json_payload = moisture_sensor_as_json(sensor);

    esp_http_client_config_t config = {
        .url = CONTROLLER_URL,
        .method = HTTP_METHOD_POST,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_payload, strlen(json_payload));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "POST success: %d", esp_http_client_get_status_code(client));
    } 
    else {
        ESP_LOGE(TAG, "POST failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

void app_main(void){
    ESP_LOGI(TAG, "Initializing sensor node...");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    wifi_init_sta();
    MoistureSensor sensor1;
    moisture_sensor_init(&sensor1, MOISTURE_IN_D32, MOISTURE_OUT_D4, 1);

    while (1) {
        moisture_sensor_update(&sensor1);
        int raw_level = moisture_sensor_get_raw(&sensor1);
        float percent = moisture_sensor_get_percent(&sensor1);
        ESP_LOGI(TAG, "Moisture Reading - Raw: %d, Percent: %.2f", raw_level, percent);

        post_moisture_data(&sensor1);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}