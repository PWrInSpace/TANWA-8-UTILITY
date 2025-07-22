#include "internal_util_config.h"


static adc_oneshot_unit_handle_t adc1_handle;   
static adc_cali_handle_t adc_cali_handle;

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

    esp_etm_channel_config_t channel_conf = {
        .gpio_num = GPIO_LED_STRIP,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = PWM_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 512,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));
}

void set_led_strip_brightness(uint32_t duty) {
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, PWM_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, PWM_CHANNEL));
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
    int voltage_mv = 0;
    // Average 10 samples for noise reduction
    for (int i = 0; i < 10; i++) {
        int raw;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL, &raw));
        adc_raw += raw;
    }
    return adc_raw /= 10;
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

void init_internal_util()
{
    configure_buzzer();
    configure_adc();
    configure_led_strip_pwm();
    configure_kontrakton_input();
    set_buzzer_state(true);
}