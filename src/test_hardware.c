#include <test_hardware.h>

volatile int stop_reading = 0;

// void stop_reading_callback(void*) {
//     ESP_LOGI(TAG_TESTS, "Stop reading callback triggered");
//     stop_reading = 1; }

int test_status_led() {
    ESP_LOGI(TAG_TESTS, "Testing status LED");
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
    return ESP_OK;
}

int test_wifi_antenna() {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_set_max_tx_power(20);
    esp_err_t err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_TESTS, "Failed to initialize WiFi: %s",
                 esp_err_to_name(err));
        return -1;
    }

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_TESTS, "Failed to set WiFi mode: %s",
                 esp_err_to_name(err));
        return -1;
    }

    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG_TESTS, "Failed to start WiFi: %s", esp_err_to_name(err));
        return -1;
    }

    err = esp_wifi_scan_start(NULL, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_TESTS, "Failed to start WiFi scan: %s",
                 esp_err_to_name(err));
        return -1;
    }

    uint16_t ap_count = 0;
    wifi_ap_record_t ap_records[20];
    err = esp_wifi_scan_get_ap_records(&ap_count, ap_records);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_TESTS, "Failed to get WiFi scan results");
        return -1;
    }

    ESP_LOGI(TAG_TESTS, "Found %d access points", ap_count);

    if (ap_count < 1) {
        ESP_LOGW(TAG_TESTS,
                 "No access points found. Antenna may be defective.");
        return -1;
    }

    for (int i = 0; i < ap_count; i++) {
        ESP_LOGI(TAG_TESTS, "AP %d: SSID: %s, RSSI: %d", i, ap_records[i].ssid,
                 ap_records[i].rssi);
    }

    return ESP_OK;
}

int test_capacitane_soil_sensor() {
    esp_err_t err;

    ledc_timer_config_t ledc_timer = {
        .freq_hz = 1000000,
        .duty_resolution = LEDC_TIMER_4_BIT,
        .timer_num = LEDC_TIMER_0,
    };

    err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_TESTS, "Failed to configure LEDC timer. %s",
                 esp_err_to_name(err));
        return -1;
    }

    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CHANNEL_0,
        .duty = 8,
        .gpio_num = PWM_PIN,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER_0,
    };

    err = ledc_channel_config(&ledc_channel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_TESTS, "Failed to configure LEDC channel. %s",
                 esp_err_to_name(err));
        return -1;
    }

    // ADC measurements
    adc_oneshot_unit_handle_t adc_handle = NULL;
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
        .unit_id = ADC_UNIT_1,
    };
    err = adc_oneshot_new_unit(&unit_cfg, &adc_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_TESTS, "Failed to create ADC unit.");
        return -1;
    }

    // ADC channel
    adc_oneshot_chan_cfg_t channel_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    err = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_4, &channel_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_TESTS, "Failed to configure ADC channel.");
        return -1;
    }

    // Set Boot Pin to listen
    gpio_config_t boot_pin_cfg = {
        .mode = GPIO_MODE_INPUT,
        // .intr_type = GPIO_INTR_ANYEDGE,
        .pin_bit_mask = (1ULL << BOOT_PIN),
    };
    err = gpio_config(&boot_pin_cfg);
    // gpio_isr_handle_t isr_handle;
    // err = err | gpio_isr_register(&stop_reading_callback, NULL,
    //                               ESP_INTR_FLAG_EDGE, &isr_handle);

    // if (err != ESP_OK) {
    //     ESP_LOGE(TAG_TESTS, "Failed to configure BOOT PIN.");
    //     return -1;
    // }

    int reading = 0;
    int min = 9999, max = 0;
    while (gpio_get_level(BOOT_PIN)) {
        err = adc_oneshot_read(adc_handle, ADC_CHANNEL_4, &reading);

        if (err != ESP_OK) {
            ESP_LOGE(TAG_TESTS, "Failed to read.");
            continue;
        }

        if (reading > max) {
            max = reading;
        } else if (reading < min) {
            min = reading;
        }

        ESP_LOGI(TAG_TESTS, "Min: %d | Max: %d | Current: %d", min, max,
                 reading);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    // Delete ADC & GPIO
    adc_oneshot_del_unit(adc_handle);
    // gpio_isr_handler_remove(BOOT_PIN);

    // Stop and delete PWM

    return ESP_OK;
}

int test_HDC3020() {
    esp_err_t err = configure_power_pin();
    err = err | power_on_peripherals();

    if (err != ESP_OK) {
        ESP_LOGE(TAG_TESTS, "Failed to provide power to peripherals.");
        return -1;
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Wait for peripherals to power up

    err = initialise_HDC3020();

    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Wait for HDC3020 to initialize

    double temp, hum;
    while (gpio_get_level(BOOT_PIN)) {
        err = err | read_HDC3020(&temp, &hum);

        if (err != ESP_OK) {
            ESP_LOGE(TAG_TESTS, "Failed to read HDC3020.");
            return -1;
        }
    }
    ESP_LOGI(TAG_TESTS, "HDC3020 test successful t: %d, h:%d", temp, hum);

    return ESP_OK;
}

void tests() {
    ESP_LOGI(TAG_TESTS, "Running hardware tests");
    test_status_led();
    esp_err_t err;
    // ESP_LOGI(TAG_TESTS, "Testing WiFi antenna");
    // err = nvs_flash_init();
    // if (err != ESP_OK) {
    //     ESP_LOGE(TAG_TESTS, "Failed to initialize NVS");
    //     return;
    // }

    // err = test_wifi_antenna();
    // if (err != ESP_OK) {
    //     ESP_LOGE(TAG_TESTS, "WiFi antenna test failed");
    // } else {
    //     ESP_LOGI(TAG_TESTS, "WiFi antenna test passed");
    // }

    err = test_capacitane_soil_sensor();

    if (err != ESP_OK) {
        ESP_LOGE(TAG_TESTS, "Capacitative Soil sensor failed.");
    }

    err = test_HDC3020();

    if (err != ESP_OK) {
        ESP_LOGE(TAG_TESTS, "HDC3020 sensor test failed.");
    }
}
