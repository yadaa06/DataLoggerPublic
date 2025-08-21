// statusled.c

#include "statusled.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "LED_DRIVER";

TaskHandle_t led_task_handle = NULL;

esp_err_t status_led_init() {
    ledc_timer_config_t ledc_timer = {
        .speed_mode      = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num       = LEDC_TIMER,
        .freq_hz         = LEDC_FREQUENCY,
        .clk_cfg         = LEDC_AUTO_CLK};

    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel[3] = {
        {.gpio_num   = LED_RED_GPIO,
         .speed_mode = LEDC_MODE,
         .channel    = LEDC_CHANNEL_R,
         .intr_type  = LEDC_INTR_DISABLE,
         .timer_sel  = LEDC_TIMER,
         .duty       = 0,
         .hpoint     = 0},
        {.gpio_num   = LED_GREEN_GPIO,
         .speed_mode = LEDC_MODE,
         .channel    = LEDC_CHANNEL_G,
         .intr_type  = LEDC_INTR_DISABLE,
         .timer_sel  = LEDC_TIMER,
         .duty       = 0,
         .hpoint     = 0},
        {.gpio_num   = LED_BLUE_GPIO,
         .speed_mode = LEDC_MODE,
         .channel    = LEDC_CHANNEL_B,
         .intr_type  = LEDC_INTR_DISABLE,
         .timer_sel  = LEDC_TIMER,
         .duty       = 0,
         .hpoint     = 0}};

    for (int i = 0; i < 3; i++) {
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel[i]));
    }

    return ESP_OK;
}

static void _led_set_color(uint32_t r, uint32_t g, uint32_t b) {
    uint32_t red_duty   = r;
    uint32_t green_duty = g;
    uint32_t blue_duty  = b;

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_R, red_duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_R);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_G, green_duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_G);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_B, blue_duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_B);
}

void error_blink_task(void* pvParameters) {
    while (1) {
        _led_set_color(MAX_DUTY, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
        _led_set_color(0, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void status_led_set_state(status_led_state_t state) {
    if (led_task_handle != NULL) {
        vTaskDelete(led_task_handle);
        led_task_handle = NULL;
    }

    switch (state) {
    case STATUS_LED_STATE_STARTING:
        ESP_LOGI(TAG, "Setting state to STARTING (RED)");
        _led_set_color(MAX_DUTY, 0, 0);
        break;
    case STATUS_LED_STATE_IN_PROGRESS:
        ESP_LOGI(TAG, "Setting state to IN PROGRESS (YELLOW)");
        _led_set_color(MAX_DUTY, MAX_DUTY * 0.6, 0);
        break;
    case STATUS_LED_STATE_READY:
        ESP_LOGI(TAG, "Setting state to READY (GREEN)");
        _led_set_color(0, MAX_DUTY, 0);
        break;
    case STATUS_LED_STATE_ERROR:
        ESP_LOGI(TAG, "Setting state to ERROR (BLINKING RED)");
        xTaskCreate(error_blink_task, "error blink", 2048, NULL, 5, &led_task_handle);
        break;
    default:
        ESP_LOGW(TAG, "Unknown LED state requested");
        _led_set_color(0, 0, 0);
        break;
    }
}