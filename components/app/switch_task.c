#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include "esp_log.h"
#include "switch_task.h"
#include "BoardData.h"
#include "can_api.h"
#include "freertos/semphr.h"

static uint8_t sw_states = 0;
static uint8_t message[3] = {0};
typedef struct {
    uint32_t gpio_num;
    bool pressed; // true for pressed, false for released
} switch_event_t;

// Structure to hold switch configuration
typedef struct {
    uint32_t gpio_num;
} switch_config_t;

switch_config_t switches[SWITCHES_QUANTITY] = {
    { .gpio_num = CONFIG_GPIO_SWITCH_1 },
    { .gpio_num = CONFIG_GPIO_SWITCH_2 },
    { .gpio_num = CONFIG_GPIO_SWITCH_3 },
    { .gpio_num = CONFIG_GPIO_SWITCH_4 },
    { .gpio_num = CONFIG_GPIO_SWITCH_5 },
    { .gpio_num = CONFIG_GPIO_SWITCH_6 },
    { .gpio_num = CONFIG_GPIO_SWITCH_7 },
    { .gpio_num = CONFIG_GPIO_SWITCH_8 }
};

esp_err_t parse_bool_to_uint8_t(const bool *states, uint8_t num_states, uint8_t *result)
{
    if (states == NULL || result == NULL || num_states > 8) {
        ESP_LOGE("SWITCH TASK", "Invalid input or too many states for uint8_t");
        return ESP_ERR_INVALID_ARG;
    }

    *result = 0; // Initialize result
    for (int i = 0; i < num_states; i++) {
        if (states[i]) {
            *result |= (1 << i); // Set i-th bit if state is true
        }
    }
    return ESP_OK;
}

// Queue to send switch events from ISR to task
static QueueHandle_t switch_queue;

// ISR handler for switch interrupts
static void IRAM_ATTR switch_isr(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    switch_event_t event = {
        .gpio_num = gpio_num,
        .pressed = (gpio_get_level(gpio_num) == 0) // Active-low: 0 means pressed
    };

    // Use a switch statement to update the appropriate switch state
    switch (gpio_num)
    {
        case CONFIG_GPIO_SWITCH_1:
            xSemaphoreTakeFromISR(BoardDataSemaphore, NULL);
            BoardData.SWITCH_STATES[0] = event.pressed;
            xSemaphoreGiveFromISR(BoardDataSemaphore, NULL);
            break;

        case CONFIG_GPIO_SWITCH_2:
            xSemaphoreTakeFromISR(BoardDataSemaphore, NULL);
            BoardData.SWITCH_STATES[1] = event.pressed; // Fixed: Use event.pressed instead of true
            xSemaphoreGiveFromISR(BoardDataSemaphore, NULL);
            break;

        case CONFIG_GPIO_SWITCH_3:
            xSemaphoreTakeFromISR(BoardDataSemaphore, NULL);
            BoardData.SWITCH_STATES[2] = event.pressed;
            xSemaphoreGiveFromISR(BoardDataSemaphore, NULL);
            break;

        case CONFIG_GPIO_SWITCH_4:
            xSemaphoreTakeFromISR(BoardDataSemaphore, NULL);
            BoardData.SWITCH_STATES[3] = event.pressed;
            xSemaphoreGiveFromISR(BoardDataSemaphore, NULL);
            break;

        case CONFIG_GPIO_SWITCH_5:
            xSemaphoreTakeFromISR(BoardDataSemaphore, NULL);
            BoardData.SWITCH_STATES[4] = event.pressed;
            xSemaphoreGiveFromISR(BoardDataSemaphore, NULL);
            break;

        case CONFIG_GPIO_SWITCH_6:
            xSemaphoreTakeFromISR(BoardDataSemaphore, NULL);
            BoardData.SWITCH_STATES[5] = event.pressed;
            xSemaphoreGiveFromISR(BoardDataSemaphore, NULL);
            break;

        case CONFIG_GPIO_SWITCH_7:
            xSemaphoreTakeFromISR(BoardDataSemaphore, NULL);
            BoardData.SWITCH_STATES[6] = event.pressed;
            xSemaphoreGiveFromISR(BoardDataSemaphore, NULL);
            break;

        case CONFIG_GPIO_SWITCH_8:
            xSemaphoreTakeFromISR(BoardDataSemaphore, NULL);
            BoardData.SWITCH_STATES[7] = event.pressed;
            xSemaphoreGiveFromISR(BoardDataSemaphore, NULL);
            break;

        default:
        break;
    }

    BaseType_t higher_priority_task_woken = pdFALSE;
    xQueueSendFromISR(switch_queue, &event, &higher_priority_task_woken);

    if (higher_priority_task_woken) {
        portYIELD_FROM_ISR();
    }
}

// Task to process switch events
static void switch_task(void *arg)
{
    switch_event_t event;
    while (1) {
        if (xQueueReceive(switch_queue, &event, portMAX_DELAY)) {
            parse_bool_to_uint8_t(BoardData.SWITCH_STATES,8,&sw_states);
            message[2] = sw_states;
            can_send_message(CAN_SEND_STATUS,message,sizeof(message));
        }
    }
}

// Function to initialize GPIOs and setup interrupts
esp_err_t switch_interrupts_init(void)
{
    switch_queue = xQueueCreate(10, sizeof(switch_event_t));
    if (!switch_queue) {
        ESP_LOGE("SWITCH", "Failed to create switch queue");
        return ESP_LOG_ERROR;
    }

    for (uint8_t i = 0; i < SWITCHES_QUANTITY; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << switches[i].gpio_num,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_ANYEDGE
        };
        gpio_config(&io_conf);

        gpio_install_isr_service(0); // only needs to be called once
        gpio_isr_handler_add(switches[i].gpio_num, switch_isr, (void *)switches[i].gpio_num);
    }

    xTaskCreate(switch_task, "switch_task", 2048, NULL, 10, NULL);
    return ESP_OK;
}
