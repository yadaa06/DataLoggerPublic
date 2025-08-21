// button.h

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#define BUTTON_GPIO GPIO_NUM_18
#define DEBOUNCE_TIME_MS 200

extern SemaphoreHandle_t xButtonSignaler;

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t button_init(void);

#ifdef __cplusplus
}
#endif