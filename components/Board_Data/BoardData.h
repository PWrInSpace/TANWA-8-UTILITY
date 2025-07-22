#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "stdbool.h"
#include "switches_ext.h"

typedef struct {
    float temperature;
    uint16_t voltage_mV;
    uint16_t curr_mA;
    bool SWITCH_STATES[SWITCHES_QUANTITY];
} BoardData_t;

extern BoardData_t BoardData;
extern SemaphoreHandle_t BoardDataSemaphore;