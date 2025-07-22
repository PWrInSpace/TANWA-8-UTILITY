#include "internal_util_config.h"
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>

// Placeholder definitions for undefined constants (adjust as needed)
#ifndef PWM_RESOLUTION
#define PWM_RESOLUTION LEDC_TIMER_10_BIT // 10-bit resolution for PWM
#endif
#ifndef PWM_FREQ_HZ
#define PWM_FREQ_HZ 5000 // 5 kHz PWM frequency
#endif
#ifndef GPIO_LED_STRIP
#define GPIO_LED_STRIP GPIO_NUM_18 // Example GPIO for LED strip
#endif
#ifndef PWM_CHANNEL
#define PWM_CHANNEL LEDC_CHANNEL_0 // LEDC channel 0
#endif
#ifndef ADC_ATTEN
#define ADC_ATTEN ADC_ATTEN_DB_12 // Use ADC_ATTEN_DB_12 (replaces deprecated ADC_ATTEN_DB_11)
#endif
#ifndef ADC_WIDTH
#define ADC_WIDTH ADC_WIDTH_BIT_12 // 12-bit ADC resolution
#endif
#ifndef ADC_CHANNEL
#define ADC_CHANNEL ADC1_CHANNEL_0 // ADC1 channel 0 (e.g., GPIO 36)
#endif
#ifndef CONFIG_GPIO_BUZZER
#define CONFIG_GPIO_BUZZER GPIO_NUM_19 // Example GPIO for buzzer
#endif
#ifndef CONFIG_GPIO_KONTRAKTON
#define CONFIG_GPIO_KONTRAKTON GPIO_NUM_21 // Example GPIO for kontrakton
#endif

adc_oneshot_unit_handle_t adc1_handle;
adc_cali_handle_t adc_cali_handle;

void configure_led_strip_pwm(void) {
    // Configure LEDC timer
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = PWM_RESOLUTION,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    // Configure LEDC channel
    ledc_channel_config_t channel_conf = {
        .gpio_num = GPIO_LED_STRIP,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = PWM_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 512, // Initial duty cycle (50% for 10-bit resolution)
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));
}

void set_led_strip_brightness(uint32_t duty) {
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL));
}

void configure_buzzer(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_GPIO_BUZZER),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_set_level(CONFIG_GPIO_BUZZER, 0));
}

void set_buzzer_state(bool state) {
    ESP_ERROR_CHECK(gpio_set_level(CONFIG_GPIO_BUZZER, state ? 1 : 0));
}

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
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_WIDTH,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle));
}

int read_led_intensity(void) {
    int adc_raw = 0;
    // Average 10 samples for noise reduction
    for (int i = 0; i < 10; i++) {
        int raw;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL, &raw));
        adc_raw += raw;
    }
    adc_raw /= 10;
    return adc_raw; // Return raw ADC value
}

void configure_kontrakton_input(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_GPIO_KONTRAKTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
}

void init_internal_util(void) {
    configure_buzzer();
    configure_adc();
    configure_led_strip_pwm();
    configure_kontrakton_input();
    set_buzzer_state(true);
}