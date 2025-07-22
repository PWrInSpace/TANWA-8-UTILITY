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

#define TAGS "UTILL_TASK"

void util_task(void *arg) {
init_internal_util();
    while(1) {
        bool kontrakton_state = gpio_get_level(CONFIG_GPIO_KONTRAKTON);
        int voltage = read_led_intensity();
        uint32_t duty = (voltage * 1023) / 4096;
        if(duty>255)
        {
            duty = 255;
        }
        duty = 255-duty;
        if(kontrakton_state == true)
        {
            set_led_strip_brightness(duty);
        }
        else{
            set_led_strip_brightness(0);
        }

        ESP_LOGI(TAGS, "ADC Voltage: %d, and duty = %d", voltage, duty);
        vTaskDelay(10 / portTICK_PERIOD_MS);
        
    }
}

esp_err_t util_task_init()
{
    if(xTaskCreatePinnedToCore(util_task, "util_task", 2048, NULL, 5, NULL, 1) == pdPASS) {
        ESP_LOGI(TAGS, "UTIL task created successfully");
    } else {
        ESP_LOGE(TAGS, "Failed to create UTIL task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

