#include <stdio.h>
#include "internal_util_config.h"

// Logging tag
static const char *TAG = "UTIL_CONFIG";

// Global variables
adc_oneshot_unit_handle_t adc1_handle = NULL;
adc_cali_handle_t adc_cali_handle = NULL;

esp_err_t configure_led_strip_pwm(void) {
    // Configure LEDC timer
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = PWM_RESOLUTION,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    esp_err_t ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC timer config failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "LEDC timer configured");

    // Configure LEDC channel
    ledc_channel_config_t channel_conf = {
        .gpio_num = GPIO_LED_STRIP,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = PWM_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 50, // Initial duty cycle (50% for 10-bit)
        .hpoint = 0
    };
    ret = ledc_channel_config(&channel_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC channel config failed on GPIO %d: %s", GPIO_LED_STRIP, esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "LEDC channel configured on GPIO %d", GPIO_LED_STRIP);

    return ESP_OK;
}

esp_err_t set_led_strip_brightness(uint32_t duty) {
    esp_err_t ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL, duty);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC set duty failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ret = ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC update duty failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "LED brightness set to %u", duty);
    return ESP_OK;
}

void configure_buzzer(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_BUZZER),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Buzzer GPIO config failed on GPIO %d: %s", GPIO_BUZZER, esp_err_to_name(ret));
    }
    ESP_ERROR_CHECK(ret);
    ret = gpio_set_level(GPIO_BUZZER, 1); // Initial state ON per original code
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Buzzer set level failed: %s", esp_err_to_name(ret));
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "Buzzer configured on GPIO %d", GPIO_BUZZER);
}

void set_buzzer_state(bool state) {
    esp_err_t ret = gpio_set_level(GPIO_BUZZER, state ? 1 : 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Buzzer set state failed: %s", esp_err_to_name(ret));
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "Buzzer state set to %d", state);
}

esp_err_t configure_adc(void) {
    // Initialize ADC1
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &adc1_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC unit initialization failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure ADC channel
    adc_oneshot_chan_cfg_t chan_config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_WIDTH,
    };
    ret = adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &chan_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC channel configuration failed on GPIO %d: %s", ADC_CHANNEL, esp_err_to_name(ret));
        if (adc1_handle != NULL) {
            adc_oneshot_del_unit(adc1_handle);
            adc1_handle = NULL;
        }
        return ret;
    }

    // Initialize ADC calibration
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_WIDTH,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ADC calibration initialized on GPIO %d", ADC_CHANNEL);
    } else {
        ESP_LOGW(TAG, "ADC calibration not available on GPIO %d", ADC_CHANNEL);
        adc_cali_handle = NULL;
    }
    return ESP_OK;
}

int read_led_intensity(void) {
    if (adc1_handle == NULL) {
        ESP_LOGE(TAG, "ADC handle is NULL, attempting reinitialization");
        esp_err_t ret = configure_adc();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "ADC reinitialization failed: %s", esp_err_to_name(ret));
            return 0;
        }
    }
    int adc_raw = 0;
    int valid_samples = 0;
    for (int i = 0; i < 10; i++) {
        int raw = 0;
        esp_err_t ret = adc_oneshot_read(adc1_handle, ADC_CHANNEL, &raw);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "ADC read failed: %s", esp_err_to_name(ret));
            continue;
        }
        adc_raw += raw;
        valid_samples++;
        vTaskDelay(10 / portTICK_PERIOD_MS); // 10ms delay between readings
    }
    if (valid_samples > 0) {
        adc_raw /= valid_samples; // Average valid samples
    } else {
        ESP_LOGE(TAG, "No valid ADC samples collected");
        adc_raw = 0;
    }

    if (adc_cali_handle) {
        int voltage_mv = 0;
        esp_err_t ret = adc_cali_raw_to_voltage(adc_cali_handle, adc_raw, &voltage_mv);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "LED intensity: %d mV", voltage_mv);
            return voltage_mv;
        } else {
            ESP_LOGE(TAG, "ADC voltage conversion failed: %s", esp_err_to_name(ret));
        }
    }
    ESP_LOGI(TAG, "LED intensity (raw): %d", adc_raw);
    return adc_raw; // Fallback to raw value if calibration fails
}

void cleanup_adc(void) {
    if (adc_cali_handle != NULL) {
        esp_err_t ret = adc_cali_delete_scheme_curve_fitting(adc_cali_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "ADC calibration cleanup failed: %s", esp_err_to_name(ret));
        }
        adc_cali_handle = NULL;
    }
    if (adc1_handle != NULL) {
        esp_err_t ret = adc_oneshot_del_unit(adc1_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "ADC unit cleanup failed: %s", esp_err_to_name(ret));
        }
        adc1_handle = NULL;
    }
    ESP_LOGI(TAG, "ADC cleaned up");
}

void configure_kontrakton_input(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_KONTRAKTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Kontrakton GPIO config failed on GPIO %d: %s", GPIO_KONTRAKTON, esp_err_to_name(ret));
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "Kontrakton input configured on GPIO %d", GPIO_KONTRAKTON);
}

esp_err_t init_internal_util(void) {
    configure_buzzer();
    esp_err_t ret = configure_adc();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC configuration failed, aborting initialization");
        return ret;
    }
    ret = configure_led_strip_pwm();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC configuration failed, aborting initialization");
        cleanup_adc();
        return ret;
    }
    configure_kontrakton_input();
    set_buzzer_state(true);
    vTaskDelay(100 / portTICK_PERIOD_MS); // 100ms delay for hardware stabilization
    ESP_LOGI(TAG, "Internal utility initialization completed");
    return ESP_OK;
}