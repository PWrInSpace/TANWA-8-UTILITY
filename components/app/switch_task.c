#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include "esp_log.h"
#include "switch_task.h"

// Type for switch callbacks
typedef void (*switch_callback_t)(void);

// Structure to hold switch event data
typedef struct {
    uint32_t gpio_num;
    bool pressed; // true for pressed, false for released
} switch_event_t;

// Structure to hold switch configuration
typedef struct {
    uint32_t gpio_num;
    switch_callback_t on_callback;
    switch_callback_t off_callback;
} switch_config_t;

// Define switch commands
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

// Switch configuration array
static const switch_config_t switch_configs[] = {
    {CONFIG_GPIO_SWITCH_1, cmd1, cmd1_off},
    {CONFIG_GPIO_SWITCH_2, cmd2, cmd2_off},
    {CONFIG_GPIO_SWITCH_3, cmd3, cmd3_off},
    {CONFIG_GPIO_SWITCH_4, cmd4, cmd4_off},
    {CONFIG_GPIO_SWITCH_5, cmd5, cmd5_off},
    {CONFIG_GPIO_SWITCH_6, cmd6, cmd6_off},
    {CONFIG_GPIO_SWITCH_7, cmd7, cmd7_off},
    {CONFIG_GPIO_SWITCH_8, cmd8, cmd8_off}
};
static const uint8_t NUM_SWITCHES = sizeof(switch_configs) / sizeof(switch_config_t);

// Queue to send switch events from ISR to task
static QueueHandle_t switch_queue;

// ISR handler for switch interrupts
static void IRAM_ATTR switch_isr(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    switch_event_t event = {
        .gpio_num = gpio_num,
        .pressed = gpio_get_level(gpio_num) == 0 // Active-low: 0 means pressed
    };
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
            for (uint8_t i = 0; i < NUM_SWITCHES; i++) {
                if (switch_configs[i].gpio_num == event.gpio_num) {
                    if (event.pressed && switch_configs[i].on_callback) {
                        switch_configs[i].on_callback();
                    } else if (!event.pressed && switch_configs[i].off_callback) {
                        switch_configs[i].off_callback();
                    }
                    break;
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
        return ESP_LOG_ERROR;
    }

    for (uint8_t i = 0; i < NUM_SWITCHES; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << switch_configs[i].gpio_num,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_ANYEDGE
        };
        gpio_config(&io_conf);

        gpio_install_isr_service(0); // only needs to be called once
        gpio_isr_handler_add(switch_configs[i].gpio_num, switch_isr, (void *)switch_configs[i].gpio_num);
    }

    xTaskCreate(switch_task, "switch_task", 2048, NULL, 10, NULL);
    return ESP_OK;
}
