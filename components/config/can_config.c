#include "can_config.h"
#include "can_api.h"
#include "can_commands.h"

#include "esp_log.h"
#include "esp_err.h"

#include "driver/twai.h"
#include "internal_util_config.h"

#define TAG "CAN_CONFIG"


esp_err_t new_command_handler(uint8_t *data, uint8_t length) {
    // Example: just print received data
    printf("New command received with length %d\n", length);
    for (int i = 0; i < length; ++i) {
        printf("Byte %d: %02X\n", i, data[i]);
    }

    // Return success
    return ESP_OK;
}


esp_err_t buzzer_handler(uint8_t *data, uint8_t length) {
    if (length < 1) {
        ESP_LOGE(TAG, "Buzzer command received with insufficient data");
        return ESP_ERR_INVALID_ARG;
    }

    // 1. Buzzer działa tradycyjnie na podstawie przesłanych danych (sztywny stan)
    bool toggle = data[0];
    uint8_t freq_s = (length > 1) ? data[1] : 0; // zabezpieczenie przed brakiem drugiego bajtu
    
    set_buzzer_state(toggle);
    gpio_set_level(GPIO_BUZZER, toggle ? 1 : 0);
    ESP_LOGI(TAG, "Buzzer state set to %s", toggle ? "ON" : "OFF");

    // 2. Dioda LED na GPIO 46 zmienia stan na przeciwny (Toggle)
    // Odczytujemy aktualny stan pinu 46, negujemy go i zapisujemy z powrotem
    int current_led_state = gpio_get_level(46);
    int new_led_state = !current_led_state;
    gpio_set_level(46, new_led_state);
    
    ESP_LOGI(TAG, "LED (GPIO 46) toggled from %d to %d", current_led_state, new_led_state);

    return ESP_OK;
}

esp_err_t status_cmd_handler(uint8_t *data, uint8_t length) {

    uint8_t data_send[3] = {0};
    data_send[0] = 0;
    data_send[1] = 0;
    data_send[2] = 0;
    
    can_send_message(CAN_NEW_SEND_STATUS_ID, data_send, 8);
    return ESP_OK;
}

can_command_t can_commands[] = {
    // Example command registration
    {CAN_SEND_STATUS, new_command_handler},
    {CAN_UTIL_GET_STATUS_ID, status_cmd_handler},
    {CAN_BUZZER_TOGGLE, buzzer_handler},
    // Add your CAN commands here
};

esp_err_t can_config_init(void) {
    esp_err_t err;

    // Register CAN commands
    err = can_register_commands(can_commands, sizeof(can_commands) / sizeof(can_commands[0]));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "CAN command registration failed");
        return err;
    }

    // Initialize CAN driver
    err = can_task_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "CAN driver initialization failed");
        return err;
    }

    // Start CAN driver
    err = can_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "CAN driver start failed");
        return err;
    }

    return ESP_OK;
}