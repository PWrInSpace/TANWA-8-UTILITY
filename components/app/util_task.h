#pragma once

#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "board_config.h"
#include "setup_task.h"
#include <driver/ledc.h>
#include "internal_util_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "board_config.h"
#include "setup_task.h"
#include <driver/ledc.h>

#define TAGS "UTIL_TASK"
#define MAX_VOLTAGE_POT 3050 // Maximum potentiometer voltage in mV
#define MIN_VOLTAGE_POT 2500 // Minimum potentiometer voltage for max duty
#define MAX_DUTY 75 // Maximum duty cycle (capped)



void util_task(void *arg) {
    init_internal_util();
    while (1) {
        bool kontrakton_state = gpio_get_level(CONFIG_GPIO_KONTRAKTON);
        int voltage = read_led_intensity(); // Returns mV or raw ADC value
        // Invert and clamp voltage: 3050 mV -> 0 duty, 2300 mV or less -> MAX_DUTY
        int voltage_reversed = MAX_VOLTAGE_POT - voltage;
        if (voltage_reversed < 0) voltage_reversed = 0; // Clamp negative values
        if (voltage <= MIN_VOLTAGE_POT) voltage_reversed = MAX_VOLTAGE_POT - MIN_VOLTAGE_POT; // Clamp at max duty
        // Scale to 0-150 duty: (voltage_reversed * MAX_DUTY) / (3050 - 2300)
        uint32_t duty = (voltage_reversed * MAX_DUTY) / (MAX_VOLTAGE_POT - MIN_VOLTAGE_POT);
        duty = MAX_DUTY-duty;
        set_led_strip_brightness(256);
        //ESP_LOGI(TAGS, "ADC Voltage: %d mV, PWM Duty: %d", voltage, duty);
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}


esp_err_t util_task_init()
{
    if(xTaskCreatePinnedToCore(util_task, "util_task", 4096, NULL, 5, NULL, 1) == pdPASS) {
        ESP_LOGI(TAGS, "UTIL task created successfully");
    } else {
        ESP_LOGE(TAGS, "Failed to create UTIL task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

