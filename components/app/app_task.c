#include "app_task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tmp1075.h"
#include "esp_log.h"
#include "board_config.h"
#include "BoardData.h"
#include "can_api.h"


#define APP_TASK_STACK_SIZE CONFIG_APP_TASK_STACK_SIZE
#define APP_TASK_PRIORITY CONFIG_APP_TASK_PRIORITY
#define APP_TASK_CORE_ID CONFIG_APP_TASK_CORE_ID
#define TAG "APP_TASK"

static TaskHandle_t app_task_handle = NULL;

esp_err_t app_task_init(void) {
    
    if(xTaskCreatePinnedToCore(app_task, "app_task", APP_TASK_STACK_SIZE, NULL, APP_TASK_PRIORITY, &app_task_handle, APP_TASK_CORE_ID) == pdPASS) {
        ESP_LOGI("APP_TASK", "App task created successfully");
    } else {
        ESP_LOGE("APP_TASK", "Failed to create app task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t app_task_deinit(void) {
    if (app_task_handle != NULL) {
        vTaskDelete(app_task_handle);
        app_task_handle = NULL;
    }
    
    return ESP_OK;
}


void app_task(void *arg) {

    // YOUR IMAGINATION IS THE ONLY LIMITATION
    while(1) {
        tmp1075_get_temp_celsius(&config.tmp1075, &BoardData.status_temp);
        led_toggle(&(config.status_led));
        vTaskDelay(250 / portTICK_PERIOD_MS);
        
    }
}