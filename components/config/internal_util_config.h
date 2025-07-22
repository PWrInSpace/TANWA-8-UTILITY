#pragma once

#define TAG "INTERNAL_UTILL_CONFIG"

#include <stdbool.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include "internal_util_config.h"

#include "driver/gpio.h"
#include "rom/gpio.h"
#include "soc/gpio_struct.h"

// Configuration macros
#define GPIO_LED_STRIP CONFIG_GPIO_LED_STRIP
#define PWM_FREQ_HZ 5000 // 5 kHz PWM frequency
#define PWM_CHANNEL LEDC_CHANNEL_0
#define PWM_RESOLUTION LEDC_TIMER_10_BIT // 10-bit resolution (0-1023)
#define GPIO_LED_INTENS 5
#define ADC_CHANNEL ADC_CHANNEL_4 // GPIO 5 on ESP32-S3 (ADC1_CHANNEL_4)
#define ADC_ATTEN ADC_ATTEN_DB_11 // 0-3.3V range
#define ADC_WIDTH ADC_BITWIDTH_12 // 12-bit resolution (0-4095)

extern adc_oneshot_unit_handle_t adc1_handle;
extern adc_cali_handle_t adc_cali_handle;

// Function prototypes
void configure_led_strip_pwm(void);
void set_led_strip_brightness(uint32_t duty);
void configure_buzzer(void);
void set_buzzer_state(bool state);
void configure_adc(void);
int read_led_intensity(void);
void cleanup_adc(void);
void configure_kontrakton_input(void);
void init_internal_util(void);