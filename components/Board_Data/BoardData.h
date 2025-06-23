#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "stdbool.h"

typedef struct {
    bool SWITCH_1_STATUS;
    bool SWITCH_2_STATUS;
    bool SWITCH_3_STATUS;
    bool SWITCH_4_STATUS;
    bool SWITCH_5_STATUS;
    bool SWITCH_6_STATUS;
    bool SWITCH_7_STATUS;
    bool SWITCH_8_STATUS;
} BoardData_t;

extern BoardData_t BoardData;
extern SemaphoreHandle_t BoardDataSemaphore;