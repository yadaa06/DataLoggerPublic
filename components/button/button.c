// button.c

#include "button.h"
#include "dht11_task.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lcd_task.hpp"

static const char* TAG = "BUTTON_DRIVER";

static volatile TickType_t last_isr_tick = 0;

SemaphoreHandle_t xButtonSignaler = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    TickType_t current_tick = xTaskGetTickCountFromISR();

    if (current_tick - last_isr_tick > pdMS_TO_TICKS(DEBOUNCE_TIME_MS)) {
        last_isr_tick = current_tick;

        BaseType_t higher_priority_task = pdFALSE;
        xSemaphoreGiveFromISR(xButtonSignaler, &higher_priority_task);

        if (higher_priority_task == pdTRUE) {
            portYIELD_FROM_ISR();
        }
    }
}

esp_err_t button_init() {
    gpio_reset_pin(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_intr_type(BUTTON_GPIO, GPIO_INTR_NEGEDGE);
    gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_DISABLE);
    xButtonSignaler = xSemaphoreCreateBinary();
    if (xButtonSignaler == NULL) {
        ESP_LOGE(TAG, "FAILED TO CREATE SEMAPHORE: EXPECT UNSTABLE BEHAVIOR");
        return ESP_FAIL;
    }
    esp_err_t isr_service_result = gpio_install_isr_service(0);
    if (isr_service_result != ESP_OK && isr_service_result != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to install ISR service: %s", esp_err_to_name(isr_service_result));
    }
    ESP_ERROR_CHECK(gpio_isr_handler_add(BUTTON_GPIO, gpio_isr_handler, NULL));

    return ESP_OK;
}