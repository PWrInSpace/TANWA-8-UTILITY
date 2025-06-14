#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include "esp_log.h"

// Structure to hold switch event data
typedef struct {
    uint32_t gpio_num;
    bool pressed; // true for pressed, false for released
} switch_event_t;

// Queue to send switch events from ISR to task
static QueueHandle_t switch_queue;

// Define switch commands (example commands)
static void cmd1(void) { printf("Switch 1 pressed: Command 1 executed\n"); }
static void cmd1_off(void) { printf("Switch 1 released: Command 1 off executed\n"); }
static void cmd2(void) { printf("Switch 2 pressed: Command 2 executed\n"); }
static void cmd2_off(void) { printf("Switch 2 released: Command 2 off executed\n"); }
static void cmd3(void) { printf("Switch 3 pressed: Command 3 executed\n"); }
static void cmd3_off(void) { printf("Switch 3 released: Command 3 off executed\n"); }
static void cmd4(void) { printf("Switch 4 pressed: Command 4 executed\n"); }
static void cmd4_off(void) { printf("Switch 4 released: Command 4 off executed\n"); }
static void cmd5(void) { printf("Switch 5 pressed: Command 5 executed\n"); }
static void cmd5_off(void) { printf("Switch 5 released: Command 5 off executed\n"); }
static void cmd6(void) { printf("Switch 6 pressed: Command 6 executed\n"); }
static void cmd6_off(void) { printf("Switch 6 released: Command 6 off executed\n"); }
static void cmd7(void) { printf("Switch 7 pressed: Command 7 executed\n"); }
static void cmd7_off(void) { printf("Switch 7 released: Command 7 off executed\n"); }
static void cmd8(void) { printf("Switch 8 pressed: Command 8 executed\n"); }
static void cmd8_off(void) { printf("Switch 8 released: Command 8 off executed\n"); }

// ISR handler for switch interrupts
static void IRAM_ATTR switch_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    switch_event_t event = {
        .gpio_num = gpio_num,
        .pressed = gpio_get_level(gpio_num) == 0 // Active-low: 0 means pressed
    };
    BaseType_t higher_priority_task_woken = pdFALSE;

    // Send event to queue
    xQueueSendFromISR(switch_queue, &event, &higher_priority_task_woken);

    // Yield if a higher priority task was woken
    if (higher_priority_task_woken) {
        portYIELD_FROM_ISR();
    }
}

// Task to process switch events
static void switch_task(void *arg)
{
    switch_event_t event;
    while (1) {
        // Wait for switch event from queue
        if (xQueueReceive(switch_queue, &event, portMAX_DELAY)) {
            // Execute command based on GPIO number and state
            switch (event.gpio_num) {
                case CONFIG_GPIO_SWITCH_1:
                    event.pressed ? cmd1() : cmd1_off();
                    break;
                case CONFIG_GPIO_SWITCH_2:
                    event.pressed ? cmd2() : cmd2_off();
                    break;
                case CONFIG_GPIO_SWITCH_3:
                    event.pressed ? cmd3() : cmd3_off();
                    break;
                case CONFIG_GPIO_SWITCH_4:
                    event.pressed ? cmd4() : cmd4_off();
                    break;
                case CONFIG_GPIO_SWITCH_5:
                    event.pressed ? cmd5() : cmd5_off();
                    break;
                case CONFIG_GPIO_SWITCH_6:
                    event.pressed ? cmd6() : cmd6_off();
                    break;
                case CONFIG_GPIO_SWITCH_7:
                    event.pressed ? cmd7() : cmd7_off();
                    break;
                case CONFIG_GPIO_SWITCH_8:
                    event.pressed ? cmd8() : cmd8_off();
                    break;
                default:
                    printf("Unknown switch GPIO: %lu\n", event.gpio_num);
                    break;
            }
        }
    }
}

// Initialize switch interrupts
esp_err_t switch_interrupts_init(void)
{
    // Create queue for switch events
    switch_queue = xQueueCreate(10, sizeof(switch_event_t));
    if (switch_queue == NULL) {
        printf("Failed to create switch queue\n");
        return ESP_LOG_ERROR;
    }

    // Configure GPIO pins for switches
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_GPIO_SWITCH_1) | (1ULL << CONFIG_GPIO_SWITCH_2) |
                        (1ULL << CONFIG_GPIO_SWITCH_3) | (1ULL << CONFIG_GPIO_SWITCH_4) |
                        (1ULL << CONFIG_GPIO_SWITCH_5) | (1ULL << CONFIG_GPIO_SWITCH_6) |
                        (1ULL << CONFIG_GPIO_SWITCH_7) | (1ULL << CONFIG_GPIO_SWITCH_8),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE, // Enable pull-up resistors
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE // Trigger on both edges (press and release)
    };
    gpio_config(&io_conf);

    // Install ISR service
    gpio_install_isr_service(0);

    // Hook ISR handler for each switch
    gpio_isr_handler_add(CONFIG_GPIO_SWITCH_1, switch_isr_handler, (void *)CONFIG_GPIO_SWITCH_1);
    gpio_isr_handler_add(CONFIG_GPIO_SWITCH_2, switch_isr_handler, (void *)CONFIG_GPIO_SWITCH_2);
    gpio_isr_handler_add(CONFIG_GPIO_SWITCH_3, switch_isr_handler, (void *)CONFIG_GPIO_SWITCH_3);
    gpio_isr_handler_add(CONFIG_GPIO_SWITCH_4, switch_isr_handler, (void *)CONFIG_GPIO_SWITCH_4);
    gpio_isr_handler_add(CONFIG_GPIO_SWITCH_5, switch_isr_handler, (void *)CONFIG_GPIO_SWITCH_5);
    gpio_isr_handler_add(CONFIG_GPIO_SWITCH_6, switch_isr_handler, (void *)CONFIG_GPIO_SWITCH_6);
    gpio_isr_handler_add(CONFIG_GPIO_SWITCH_7, switch_isr_handler, (void *)CONFIG_GPIO_SWITCH_7);
    gpio_isr_handler_add(CONFIG_GPIO_SWITCH_8, switch_isr_handler, (void *)CONFIG_GPIO_SWITCH_8);

    // Create task to handle switch events
    xTaskCreate(switch_task, "switch_task", CONFIG_APP_TASK_STACK_SIZE, NULL,
                CONFIG_APP_TASK_PRIORITY, NULL);

    printf("Switch interrupts initialized\n");

    return ESP_OK;
}