#include "HDC3020.h"

gpio_config_t power_pin_config = {
    .mode = GPIO_MODE_OUTPUT,
    .pin_bit_mask = (1ULL << PERIPHERAL_POWER_GPIO),
};

i2c_master_bus_config_t i2c_config = {
    .scl_io_num = GPIO_NUM_2,
    .sda_io_num = GPIO_NUM_3,
    .clk_source = I2C_CLK_SRC_DEFAULT,
};

i2c_device_config_t HDC3020_i2c_config = {.dev_addr_length = I2C_ADDR_BIT_LEN_7,
                                          .device_address = 0x44,
                                          .scl_speed_hz = 400000};

i2c_master_bus_handle_t i2c_handle;
i2c_master_dev_handle_t HDC3020_handle;

esp_err_t configure_power_pin() { return gpio_config(&power_pin_config); }

esp_err_t power_on_peripherals() {
    return gpio_set_level(PERIPHERAL_POWER_GPIO, 1);
}

esp_err_t power_off_peripherals() {
    return gpio_set_level(PERIPHERAL_POWER_GPIO, 0);
}

int get_peripheral_power_state() {
    return gpio_get_level(PERIPHERAL_POWER_GPIO);
}

esp_err_t initialise_HDC3020() {
    if (get_peripheral_power_state() != 1) {
        ESP_ERROR_CHECK(power_on_peripherals());
    }

    esp_err_t err = i2c_new_master_bus(&i2c_config, &i2c_handle);
    err = err | i2c_master_bus_add_device(i2c_handle, &HDC3020_i2c_config,
                                          &HDC3020_handle);
    if (err != ESP_OK) {
        ESP_LOGE(HDC3020, "Failed to start i2c device.");
        return -1;
    }

    return ESP_OK;
}

esp_err_t read_HDC3020(double* temp, double* hum) {
    uint8_t command[2] = {0x24, 0x00};
    esp_err_t err = i2c_master_transmit(HDC3020_handle, command, 2, 500);

    vTaskDelay(500 / portTICK_PERIOD_MS);

    uint8_t readings[4] = {0};

    err = err | i2c_master_receive(HDC3020_handle, readings,
                                   (4 * sizeof(uint8_t)), 500);

    if (err != ESP_OK) {
        ESP_LOGE(HDC3020, "Error reading sensor.");
        return -1;
    } else {
        uint16_t raw_temp = ((uint16_t)readings[0] << 8) | readings[1];
        uint16_t raw_hum = ((uint16_t)readings[2] << 8) | readings[3];

        *temp = -45.0 + (175.0 * ((double)raw_temp / 65535.0));
        *hum = 100.0 * ((double)raw_hum / 65535.0);

        ESP_LOGI(HDC3020, "Successful readings temp: %d and  hum: %d.", *temp,
                 *hum);
    }
    return ESP_OK;
}

esp_err_t delete_HDC3020() {
    esp_err_t err = i2c_master_bus_rm_device(HDC3020_handle);
    err = err | i2c_del_master_bus(i2c_handle);
    return err;
}
