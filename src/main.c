#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>

#include "hardware.h"
#include "test_hardware.h"

#define TAG "main"

void blink_status_led(void* pvParameters) {
    while (1) {
        gpio_set_level(STATUS_LED, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(STATUS_LED, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void app_main() {
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << STATUS_LED),
    };
    gpio_config(&io_conf);
    gpio_set_level(STATUS_LED, 0);
    
    for (int i = 0; i < 10; i++) {
        ESP_LOGI(TAG_TESTS, "Test %d of 10", i + 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "Application booted");
    
    // TaskHandle_t xblinker_handle = NULL;
    // xTaskCreate(blink_status_led, "blink_status_led", 2048, NULL, 5,
    //             &xblinker_handle);

    tests();
}