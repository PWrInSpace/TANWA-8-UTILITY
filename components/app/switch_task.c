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

#define DEBOUNCE_TIME_MS 50 // Debounce time in milliseconds

static uint8_t sw_states = 0;
static uint8_t message[3] = {0};
typedef struct {
    uint32_t gpio_num;
    bool pressed; // true for pressed, false for released
    TickType_t last_event_time; // Last time this switch was processed
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

    *result = 0;
    for (int i = 0; i < num_states; i++) {
        if (states[i]) {
            *result |= (1 << i);
        }
    }
    return ESP_OK;
}

// Queue to send switch events from ISR to task
static QueueHandle_t switch_queue;
static TickType_t last_switch_time[SWITCHES_QUANTITY] = {0}; // Track last event time for each switch

// ISR handler for switch interrupts
static void IRAM_ATTR switch_isr(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    switch_event_t event = {
        .gpio_num = gpio_num,
        .pressed = (gpio_get_level(gpio_num) == 0), // Active-low: 0 means pressed
        .last_event_time = xTaskGetTickCountFromISR()
    };

    // Update BoardData directly without semaphore in ISR
    for (int i = 0; i < SWITCHES_QUANTITY; i++) {
        if (switches[i].gpio_num == gpio_num) {
            BoardData.SWITCH_STATES[i] = event.pressed;
            break;
        }
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
        if (xQueueReceive(switch_queue, &event, portMAX_DELAY)) { // Use portMAX_DELAY for reliability
            // Debounce: Ignore events within DEBOUNCE_TIME_MS
            int switch_idx = -1;
            for (int i = 0; i < SWITCHES_QUANTITY; i++) {
                if (switches[i].gpio_num == event.gpio_num) {
                    switch_idx = i;
                    break;
                }
            }

            if (switch_idx >= 0) {
                TickType_t current_time = xTaskGetTickCount();
                TickType_t time_diff = (current_time - last_switch_time[switch_idx]) * portTICK_PERIOD_MS;

                if (time_diff >= DEBOUNCE_TIME_MS) {
                    last_switch_time[switch_idx] = current_time;

                    // Log only the specific switch event
                    ESP_LOGI("SWITCH TASK", "Switch %d %s", event.gpio_num, event.pressed ? "pressed" : "released");

                    // Update CAN message
                    parse_bool_to_uint8_t(BoardData.SWITCH_STATES, 8, &sw_states);
                    message[0] = 200;
                    message[2] = sw_states;
                    printf("message = %d\n", message[2]);
                    can_send_message(CAN_SEND_STATUS, message, sizeof(message));
                }
            }
        }
    }
}

// Function to initialize GPIOs and setup interrupts
esp_err_t switch_interrupts_init(void)
{
    switch_queue = xQueueCreate(10, sizeof(switch_event_t));
    if (!switch_queue) {
        ESP_LOGE("SWITCH", "Failed to create switch queue");
        return ESP_FAIL; // Use ESP_FAIL for consistency
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

        // Install ISR service only once
        static bool isr_service_installed = false;
        if (!isr_service_installed) {
            gpio_install_isr_service(0);
            isr_service_installed = true;
        }
        gpio_isr_handler_add(switches[i].gpio_num, switch_isr, (void *)switches[i].gpio_num);
    }

    xTaskCreate(switch_task, "switch_task", 4096, NULL, 10, NULL);
    return ESP_OK;
    }