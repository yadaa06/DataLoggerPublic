// button_task.hpp

#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "button.h"


#ifdef __cplusplus
class Button {
private:
    static Button* s_button_instance;
    TaskHandle_t task_handle = nullptr;

    static void button_task_wrapper(void* pvParameters);
    void button_task_loop();

public:
    Button();
    ~Button();
    static Button* get_instance();

    esp_err_t start_task(BaseType_t priority, uint32_t stack_depth);
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t start_button_task(BaseType_t priority, uint32_t stack_depth);

#ifdef __cplusplus
}
#endif