#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>

#ifndef HDC3020
#define HDC3020 "HDC3020"
#define PERIPHERAL_POWER_GPIO GPIO_NUM_8

esp_err_t configure_power_pin();

esp_err_t power_on_peripherals();

esp_err_t power_off_peripherals();

int get_peripheral_power_state();

esp_err_t initialise_HDC3020();

esp_err_t read_HDC3020(double* temp, double* hum);

esp_err_t delete_HDC3020();

#endif