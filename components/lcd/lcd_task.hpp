// lcd_task.hpp

#pragma once

#include "dht11_task.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_i2c.h"

#define CYCLE_MODE 9
#define NEW_DATA 11
#define TOGGLE_POWER 10

#ifdef __cplusplus

enum class DisplayMode {
    TEMPERATURE,
    HUMIDITY,
    LAST_READ,
    MAX_MODES
};

class LCDDisplay {
  private:
    static LCDDisplay* s_lcd_instance;
    TaskHandle_t task_handle     = nullptr;
    lcd_i2c_handle_t* lcd_handle = nullptr;

    DisplayMode current_mode = DisplayMode::TEMPERATURE;
    bool is_lcd_on = true;

    void render_current_mode(DHT11Sensor* sensor);
    static void display_task_wrapper(void* pvParameters);
    void display_task_loop();

  public:
    LCDDisplay();
    ~LCDDisplay();
    static LCDDisplay* get_instance();

    void notify_new_data();
    esp_err_t start_task(BaseType_t priority, uint32_t stack_depth);
    void cycle_mode();
    void toggle_power();
    bool is_on();
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t start_lcd_display_task(BaseType_t priority, uint32_t stack_depth);
void lcd_cycle_mode();

#ifdef __cplusplus
}
#endif