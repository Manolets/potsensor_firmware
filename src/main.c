#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <esp_err.h>
#include "test_hardware.h"

#include "hardware.h"

#define TAG "main"

void blink_status_led(void *pvParameters) {
    while(1) {
        gpio_set_level(STATUS_LED, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(STATUS_LED, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

}

void app_main() {
    ESP_LOGI(TAG, "Application booted");
    TaskHandle_t xblinker_handle = NULL;
    xTaskCreate(blink_status_led, "blink_status_led", 2048, NULL, 5, &xblinker_handle);

    tests();
}