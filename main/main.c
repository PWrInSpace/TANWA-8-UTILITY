#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "board_config.h"
#include "setup_task.h"
#include <driver/ledc.h>
#define TAG "APP"

extern board_config_t config;

#define GPIO_LED_STRIP CONFIG_GPIO_LED_STRIP
#define PWM_FREQ_HZ 5000 // 5 kHz PWM frequency
#define PWM_CHANNEL LEDC_CHANNEL_0
#define PWM_RESOLUTION LEDC_TIMER_10_BIT // 10-bit resolution (0-1023)

void configure_led_strip_pwm(void) {
    // Configure LEDC timer
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = PWM_RESOLUTION,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    // Configure LEDC channel
    ledc_channel_config_t channel_conf = {
        .gpio_num = GPIO_LED_STRIP,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = PWM_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 512, // Initial duty cycle (50% for 10-bit resolution)
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));
}

// Set PWM duty cycle (0-1023 for 10-bit resolution)
void set_led_strip_brightness(uint32_t duty) {
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, PWM_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, PWM_CHANNEL));
}


void configure_buzzer(void) {
    // Configure GPIO as output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_GPIO_BUZZER),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Set buzzer to high (on)
    ESP_ERROR_CHECK(gpio_set_level(CONFIG_GPIO_BUZZER, 1));
}

// Function to turn buzzer on/off
void set_buzzer_state(bool state) {
    ESP_ERROR_CHECK(gpio_set_level(CONFIG_GPIO_BUZZER, state ? 1 : 0));
}


#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>

#define GPIO_LED_INTENS 5
#define ADC_CHANNEL ADC_CHANNEL_4 // GPIO 5 on ESP32-S3 (ADC1_CHANNEL_4)
#define ADC_ATTEN ADC_ATTEN_DB_11 // 0-3.3V range
#define ADC_WIDTH ADC_BITWIDTH_12 // 12-bit resolution (0-4095)

static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc_cali_handle;

void configure_adc(void) {
    // Initialize ADC1
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    // Configure ADC channel
    adc_oneshot_chan_cfg_t chan_config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_WIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &chan_config));

    // Initialize ADC calibration
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_WIDTH,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_config, &adc_cali_handle));
}

// Read ADC voltage in millivolts
int read_led_intensity(void) {
    int adc_raw = 0;
    int voltage_mv = 0;
    // Average 10 samples for noise reduction
    for (int i = 0; i < 10; i++) {
        int raw;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL, &raw));
        adc_raw += raw;
        vTaskDelay(10 / portTICK_PERIOD_MS); // Small delay between samples
    }
    return adc_raw /= 10;
}

// Cleanup ADC (optional, call when done with ADC)
void cleanup_adc(void) {
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(adc_cali_handle));
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
}

void configure_kontrakton_input(void) {
    // Configure GPIO as input with pull-up
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_GPIO_KONTRAKTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
}

void app_main(void) {
    ESP_LOGI(TAG, "%s TANWA board starting", config.board_name);

    if (setup_task_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize setup task");
        return;
    }

    configure_buzzer();
    configure_adc();
    configure_led_strip_pwm();
    configure_kontrakton_input();
    set_buzzer_state(true);

    while (1) {
        bool kontrakton_state = gpio_get_level(CONFIG_GPIO_KONTRAKTON);
        int voltage = read_led_intensity();
        uint32_t duty = (voltage * 1023) / 4096;
        if(kontrakton_state == false)
        {
            set_led_strip_brightness(duty);
        }
        else{
            set_led_strip_brightness(0);
        }

        ESP_LOGI(TAG, "ADC Voltage: %d, and duty = %d", voltage, duty);
        vTaskDelay(10 / portTICK_PERIOD_MS); // Update every 500ms
    }
}