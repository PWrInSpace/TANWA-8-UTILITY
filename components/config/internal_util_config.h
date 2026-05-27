#pragma once

#include <stdbool.h>
#include <esp_err.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <driver/ledc.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

// Configuration macros
#define GPIO_LED_STRIP 9 // GPIO 9 for LED strip
#define PWM_FREQ_HZ 5000 // 5 kHz PWM frequency
#define PWM_CHANNEL LEDC_CHANNEL_0
#define PWM_RESOLUTION LEDC_TIMER_12_BIT // 10-bit resolution (0-1023)
#define GPIO_BUZZER GPIO_NUM_10 // GPIO for buzzer
#define GPIO_KONTRAKTON GPIO_NUM_21 // GPIO for kontrakton
#define ADC_CHANNEL ADC_CHANNEL_5 // GPIO 5 on ESP32-S3
#define ADC_ATTEN ADC_ATTEN_DB_12 // 0-3.3V range
#define ADC_WIDTH ADC_BITWIDTH_12 // 12-bit resolution (0-4095)


// Global variables
extern adc_oneshot_unit_handle_t adc1_handle;
extern adc_cali_handle_t adc_cali_handle;

// Function prototypes
esp_err_t configure_led_strip_pwm(void);
esp_err_t set_led_strip_brightness(uint32_t duty);
void configure_buzzer(void);
void set_buzzer_state(bool state);
esp_err_t configure_adc(void);
int read_led_intensity(void);
void cleanup_adc(void);
void configure_kontrakton_input(void);
esp_err_t init_internal_util(void);