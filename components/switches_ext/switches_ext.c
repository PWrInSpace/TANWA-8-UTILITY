#include "switches_ext.h"
#include "esp_log.h"
esp_err_t sw_ext_init()
{
gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
  gpio_num_t input_pins[] = {
        CONFIG_GPIO_SWITCH_1,
CONFIG_GPIO_SWITCH_2,
CONFIG_GPIO_SWITCH_3,
CONFIG_GPIO_SWITCH_4,
CONFIG_GPIO_SWITCH_5,
CONFIG_GPIO_SWITCH_6,
CONFIG_GPIO_SWITCH_7,
CONFIG_GPIO_SWITCH_8
    };

    for (int i = 0; i < sizeof(input_pins)/sizeof(input_pins[0]); i++) {
        io_conf.pin_bit_mask = 1ULL << input_pins[i];
        gpio_config(&io_conf);
    }

    return ESP_OK;
}