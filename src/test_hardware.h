#include <driver/mcpwm_cmpr.h>
#include <driver/mcpwm_gen.h>
#include <driver/mcpwm_oper.h>
#include <driver/mcpwm_timer.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <nvs_flash.h>

#include <HDC3020.h>
#include "hardware.h"

#pragma once

#define TAG_TESTS "TESTS"

void stop_reading_callback(void*);

int test_status_led();

int test_wifi_antenna();

int test_capacitane_soil_sensor();

int test_HDC3020();

void tests();