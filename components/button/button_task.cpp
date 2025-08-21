// button_task.cpp

#include "button_task.hpp"
#include "lcd_task.hpp"
#include "esp_log.h"

static const char* TAG = "BUTTON_TASK";

Button* Button::s_button_instance = nullptr;

Button::Button() {}

Button::~Button() {
    if (task_handle) {
        vTaskDelete(task_handle);
    }
}

Button* Button::get_instance() {
    if (!s_button_instance) {
        s_button_instance = new Button();
    }
    return s_button_instance;
}

esp_err_t Button::start_task(BaseType_t priority, uint32_t stack_depth) {
    esp_err_t result = button_init();
    if (result != ESP_OK) {
        return result;
    }
    BaseType_t task_result = xTaskCreate(
        Button::button_task_wrapper,
        "button_task",
        stack_depth,
        this,
        priority,
        &task_handle
    );
    if (task_result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button task");
        return ESP_FAIL;
    }
    return ESP_OK;
}

void Button::button_task_wrapper(void* pvParameters) {
    Button* instance = static_cast<Button*>(pvParameters);
    if (instance) {
        instance->button_task_loop();
    }
    vTaskDelete(nullptr);
}

void Button::button_task_loop() {
    ESP_LOGI(TAG, "Button task started");
    while (true) {
        if (xSemaphoreTake(xButtonSignaler, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "BUTTON PRESSED");
            LCDDisplay::get_instance()->cycle_mode();
        }
    }
}

esp_err_t start_button_task(BaseType_t priority, uint32_t stack_depth) {
    Button* button = Button::get_instance();
    if (button) {
        return button->start_task(priority, stack_depth);
    }
    return ESP_FAIL;
}