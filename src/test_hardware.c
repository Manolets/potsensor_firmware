#include <driver/mcpwm_cmpr.h>
#include <driver/mcpwm_gen.h>
#include <driver/mcpwm_oper.h>
#include <driver/mcpwm_timer.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <nvs_flash.h>
#include <esp_adc/adc_oneshot.h>

#include "hardware.h"

#define TAG "TESTS"

void test_status_led() {
    ESP_LOGI(TAG, "Testing status LED");
    gpio_set_level(STATUS_LED, 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    gpio_set_level(STATUS_LED, 0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    for (int i = 0; i < 5; i++) {
        gpio_set_level(STATUS_LED, 1);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        gpio_set_level(STATUS_LED, 0);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

int test_wifi_antenna() {
    esp_err_t err = esp_wifi_scan_start(NULL, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi scan");
        return -1;
    }

    uint16_t ap_count = 0;
    wifi_ap_record_t ap_records[20];
    err = esp_wifi_scan_get_ap_records(&ap_count, ap_records);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get WiFi scan results");
        return -1;
    }

    ESP_LOGI(TAG, "Found %d access points", ap_count);

    if (ap_count < 1) {
        ESP_LOGW(TAG, "No access points found. Antenna may be defective.");
        return -1;
    }

    for (int i = 0; i < ap_count; i++) {
        ESP_LOGI(TAG, "AP %d: SSID: %s, RSSI: %d", i, ap_records[i].ssid,
                 ap_records[i].rssi);
    }

    return ESP_OK;
}

int test_capacitane_soil_sensor() {
    mcpwm_timer_handle_t timer = NULL;
    mcpwm_oper_handle_t oper = NULL;
    mcpwm_cmpr_handle_t comparator = NULL;
    mcpwm_gen_handle_t generator = NULL;

    // PWM Timer
    const mcpwm_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = 4000000,  // 4 MHz
        .period_ticks = 4,         // Period = 4 tics = 1MHz cycle
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    esp_err_t err = mcpwm_new_timer(&timer_config, &timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create PWM timer.");
        return -1;
    }

    // PWM Operator
    const mcpwm_operator_config_t oper_config = {
        .group_id = 0,
    };
    err = mcpwm_new_operator(&oper_config, &oper);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create PWM operator.");
        return -1;
    }

    // PWM Comparator
    const mcpwm_comparator_config_t comparator_config = {
        .flags.update_cmp_on_tez = true,
    };
    err = mcpwm_new_comparator(oper, &comparator_config, &comparator);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create PWM comparator.");
        return -1;
    }

    err = mcpwm_comparator_set_compare_value(comparator,
                                             2);  // 2/4 tics => 50% duty cycle
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set duty cycle for PWM comparator.");
        return -1;
    }

    // PWM Generator
    const mcpwm_generator_config_t gen_config = {
        .gen_gpio_num = PWM_PIN,
    };
    err = mcpwm_new_generator(oper, &gen_config, &generator);

    err = err | mcpwm_generator_set_action_on_timer_event(
                    oper, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                                       MCPWM_TIMER_EVENT_EMPTY,
                                                       MCPWM_GEN_ACTION_HIGH));

    err = err | mcpwm_generator_set_action_on_compare_event(
                    generator, MCPWM_GEN_COMPARE_EVENT_ACTION(
                                   MCPWM_TIMER_DIRECTION_UP, comparator,
                                   MCPWM_GEN_ACTION_LOW));

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create and configure PWM generator.");
        return -1;
    }

    // Start PWM
    err = mcpwm_timer_enable(timer);
    err = err | mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start PWM timer.");
        return -1;
    }

    // ADC measurements
    adc_oneshot_unit_handle_t adc_handle = NULL;
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
        .unit_id = ADC_UNIT_1,
    };
    err = adc_oneshot_new_unit(&unit_cfg, &adc_handle);
    if (err != ESP_OK){
        ESP_LOGE(TAG, "Failed to create ADC unit.");
        return -1;
    }

    // ADC channel
    adc_oneshot_chan_cfg_t channel_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    err = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_4, &channel_cfg);
    if (err != ESP_OK){
        ESP_LOGE(TAG, "Failed to configure ADC channel.");
        return -1;
    }

    // TODO read measurements until BOOT button is pressed.

    // Delete ADC
    adc_oneshot_del_unit(adc_handle);

    // Stop and delete PWM
    mcpwm_timer_start_stop(timer, MCPWM_TIMER_STOP_FULL);
    mcpwm_del_generator(generator);
    mcpwm_del_comparator(comparator);
    mcpwm_del_operator(oper);
    mcpwm_del_timer(timer);
}

void tests() {
    ESP_LOGI(TAG, "Running hardware tests");
    test_status_led();

    ESP_LOGI(TAG, "Testing WiFi antenna");
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS");
        return;
    }

    err = test_wifi_antenna();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi antenna test failed");
    } else {
        ESP_LOGI(TAG, "WiFi antenna test passed");
    }
}