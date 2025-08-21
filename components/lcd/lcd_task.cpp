// lcd_task.cpp

#include "lcd_task.hpp"
#include "dht11_task.hpp"
#include "webserver.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "lcd_i2c.h"

static const char* TAG = "LCD_TASK";

LCDDisplay* LCDDisplay::s_lcd_instance = nullptr;

LCDDisplay::LCDDisplay() {
}

LCDDisplay::~LCDDisplay() {
    if (task_handle) {
        vTaskDelete(task_handle);
    }
}

LCDDisplay* LCDDisplay::get_instance() {
    if (!s_lcd_instance) {
        s_lcd_instance = new LCDDisplay();
    }
    return s_lcd_instance;
}

esp_err_t LCDDisplay::start_task(BaseType_t priority, uint32_t stack_depth) {
    BaseType_t result = xTaskCreate(display_task_wrapper, "lcd_task", stack_depth, this, priority, &task_handle);

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LCD task");
        return ESP_FAIL;
    }
    return ESP_OK;
}

void LCDDisplay::cycle_mode() {
    if (task_handle) {
        xTaskNotify(task_handle, CYCLE_MODE, eSetValueWithoutOverwrite);
    }
}

void LCDDisplay::toggle_power() {
    if (task_handle) {
        xTaskNotify(task_handle, TOGGLE_POWER, eSetValueWithoutOverwrite);
        is_lcd_on ^= 1; 
        char json_buffer[64];
        snprintf(json_buffer, sizeof(json_buffer), "{\"lcd_on\": %s}", is_lcd_on ? "true" : "false");
        Webserver::get_instance()->send_state_update(json_buffer);
    }
}

bool LCDDisplay::is_on() {
    return is_lcd_on;
}

void LCDDisplay::notify_new_data() {
    xTaskNotify(task_handle, NEW_DATA, eSetValueWithoutOverwrite);
}

esp_err_t start_lcd_display_task(BaseType_t priority, uint32_t stack_depth) {
    LCDDisplay* lcd = LCDDisplay::get_instance();
    if (lcd) {
        return lcd->start_task(priority, stack_depth);
    }
    return ESP_FAIL;
}

void lcd_cycle_mode() {
    LCDDisplay* lcd = LCDDisplay::get_instance();
    if (lcd) {
        lcd->cycle_mode();
    } else {
        ESP_LOGE(TAG, "LCDDisplay instance not available!");
    }
}

void LCDDisplay::render_current_mode(DHT11Sensor* sensor) {
    vTaskDelay(pdMS_TO_TICKS(100));
    switch (current_mode) {
        case DisplayMode::TEMPERATURE: {
            float temperature = sensor->get_temperature();
            lcd_i2c_write_string(lcd_handle, "Temp: %.2f %cF", temperature, 223);
            lcd_i2c_set_cursor(lcd_handle, 0, 1);
            lcd_i2c_write_string(lcd_handle, "Next: Hum");
            break;
        }
        case DisplayMode::HUMIDITY: {
            float humidity = sensor->get_humidity();
            lcd_i2c_write_string(lcd_handle, "Hum: %.2f%%", humidity);
            lcd_i2c_set_cursor(lcd_handle, 0, 1);
            lcd_i2c_write_string(lcd_handle, "Next: LR");
            break;
        }
        case DisplayMode::LAST_READ: {
            uint64_t last_read_us    = sensor->get_last_read();
            uint64_t current_time_us = esp_timer_get_time();
            uint32_t seconds_since_last_read = (current_time_us - last_read_us) / 1000000;
            lcd_i2c_write_string(lcd_handle, "LR: %lu secs ago", seconds_since_last_read);
            lcd_i2c_set_cursor(lcd_handle, 0, 1);
            lcd_i2c_write_string(lcd_handle, "Next: Temp");
            break;
        }
        default:
            break;
    }
}

void LCDDisplay::display_task_wrapper(void* pvParameters) {
    LCDDisplay* instance = static_cast<LCDDisplay*>(pvParameters);
    if (instance) {
        instance->display_task_loop();
    }
    vTaskDelete(nullptr);
}

void LCDDisplay::display_task_loop() {
    lcd_handle = lcd_i2c_init();
    vTaskDelay(pdMS_TO_TICKS(100));

    if (!lcd_handle) {
        ESP_LOGE(TAG, "LCD INITIALIZATION FAILED");
        vTaskDelete(nullptr);
    }

    DHT11Sensor* dht11_sensor = DHT11Sensor::get_instance();

    if (!dht11_sensor) {
        ESP_LOGE(TAG, "DHT11 sensor instance not available!");
        vTaskDelete(nullptr);
    }

    while (true) {
        uint32_t ulNotifiedValue;
        BaseType_t xResult = xTaskNotifyWait(0, UINT32_MAX, &ulNotifiedValue, pdMS_TO_TICKS(5000));
        
        if (xResult == pdTRUE) {
            if (!is_lcd_on && ulNotifiedValue != TOGGLE_POWER) {
                continue;
            }
            lcd_i2c_clear(lcd_handle);
            lcd_i2c_home(lcd_handle);

            switch (ulNotifiedValue) {
                case TOGGLE_POWER:
                if (is_lcd_on) {
                    ESP_LOGI(TAG, "LCD turning ON");
                    lcd_i2c_backlight(lcd_handle, true);
                    lcd_i2c_write_string(lcd_handle, "LCD Power ON!");
                    vTaskDelay(pdMS_TO_TICKS(5000));
                    lcd_i2c_clear(lcd_handle);
                    lcd_i2c_home(lcd_handle);
                    render_current_mode(dht11_sensor);
                }
                else {
                    ESP_LOGI(TAG, "LCD turning OFF");
                    lcd_i2c_backlight(lcd_handle, false);
                }
                break;

                case CYCLE_MODE:
                ESP_LOGI(TAG, "Changing Mode");
                current_mode = static_cast<DisplayMode>((static_cast<int>(current_mode) + 1) % static_cast<int>(DisplayMode::MAX_MODES));

                [[fallthrough]];

                case NEW_DATA:
                render_current_mode(dht11_sensor);
        }
    }
}
}