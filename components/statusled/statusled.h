// statusled.h

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/gpio.h"

#define LED_RED_GPIO       GPIO_NUM_33
#define LED_GREEN_GPIO     GPIO_NUM_26
#define LED_BLUE_GPIO      GPIO_NUM_27

#define LEDC_TIMER         LEDC_TIMER_0
#define LEDC_CHANNEL_R     LEDC_CHANNEL_0
#define LEDC_CHANNEL_G     LEDC_CHANNEL_1
#define LEDC_CHANNEL_B     LEDC_CHANNEL_2

#define LEDC_MODE          LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES      LEDC_TIMER_10_BIT
#define LEDC_FREQUENCY     5000
#define MAX_DUTY           ((1 << LEDC_DUTY_RES) - 1)

typedef enum {
    STATUS_LED_STATE_STARTING,
    STATUS_LED_STATE_IN_PROGRESS,
    STATUS_LED_STATE_READY,
    STATUS_LED_STATE_ERROR
} status_led_state_t;

void status_led_set_state(status_led_state_t state);
void error_blink_task(void* pvParameters);
esp_err_t status_led_init();

#ifdef __cplusplus
}
#endif