#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

// Callback function type for switch actions
typedef void (*switch_callback_t)(void);

// Initialize switch interrupts
esp_err_t switch_interrupts_init(void);
