// dht11_task.cpp

#include "dht11_task.hpp"
#include "dht11.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "speaker_task.hpp"
#include "lcd_task.hpp"
#include <math.h>
#include <stdbool.h>
#include <time.h>

static const char* TAG = "DHT11_TASK";

DHT11Sensor* DHT11Sensor::s_dht11_instance = nullptr;

DHT11Sensor::DHT11Sensor() {
    this->mutex = xSemaphoreCreateMutex();
    if (!this->mutex) {
        ESP_LOGE(TAG, "Failed to create mutex!");
    }
}

DHT11Sensor::~DHT11Sensor() {
    if (this->mutex) {
        vSemaphoreDelete(this->mutex);
    }
    if (this->task_handle) {
        vTaskDelete(this->task_handle);
    }
}

DHT11Sensor* DHT11Sensor::get_instance() {
    if (s_dht11_instance == nullptr) {
        s_dht11_instance = new DHT11Sensor();
    }

    return s_dht11_instance;
}

esp_err_t DHT11Sensor::start_task(BaseType_t priority, uint32_t stack_depth) {
    if (!this->mutex) {
        return ESP_FAIL;
    }
    BaseType_t result = xTaskCreate(read_data_task_wrapper, "dht11_task", stack_depth, this, priority, &this->task_handle);
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create DHT11 task!");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void DHT11Sensor::notify_read() {
    if (this->task_handle) {
        xTaskNotifyGive(this->task_handle);
        ESP_LOGI(TAG, "Sent notification to DHT11 task to read NOW");
    } else {
        ESP_LOGE(TAG, "DHT11 TASK HANDLE IS NULL, CAN'T SEND NOTIF");
    }
}

float DHT11Sensor::get_temperature() {
    float temp_read = NAN;
    if (xSemaphoreTake(this->mutex, portMAX_DELAY) == pdTRUE) {
        temp_read = this->temperature;
        xSemaphoreGive(this->mutex);
    } else {
        ESP_LOGE(TAG, "ERROR: dht11_get_temperature failed to take mutex!");
    }
    return temp_read;
}

float DHT11Sensor::get_humidity() {
    float hum_read = NAN;
    if (xSemaphoreTake(this->mutex, portMAX_DELAY) == pdTRUE) {
        hum_read = this->humidity;
        xSemaphoreGive(this->mutex);
    } else {
        ESP_LOGE(TAG, "ERROR: dht11_get_humidity failed to take mutex!");
    }
    return hum_read;
}

void DHT11Sensor::get_history(dht11_reading_t* history_buffer, uint32_t* num_readings) {
    if (xSemaphoreTake(this->mutex, portMAX_DELAY) == pdTRUE) {
        *num_readings = this->num_history_readings;
        if (this->num_history_readings > 0) {
            int start_idx = (this->history_idx - this->num_history_readings + DHT_HISTORY_SIZE) % DHT_HISTORY_SIZE;
            for (int i = 0; i < this->num_history_readings; i++) {
                int current_buffer_idx = (start_idx + i) % DHT_HISTORY_SIZE;
                history_buffer[i]      = this->dht_history[current_buffer_idx];
            }
        }
        xSemaphoreGive(this->mutex);
    } else {
        ESP_LOGE(TAG, "ERROR: dht11_get_history failed to take mutex!");
        *num_readings = 0;
    }
}

uint64_t DHT11Sensor::get_last_read() {
    uint64_t time_read = 0;
    if (xSemaphoreTake(this->mutex, portMAX_DELAY) == pdTRUE) {
        time_read = this->last_successful_read;
        xSemaphoreGive(this->mutex);
    }
    return time_read;
}

esp_err_t start_dht11_sensor_task(BaseType_t priority, uint32_t stack_depth) {
    DHT11Sensor* instance = DHT11Sensor::get_instance();
    if (instance) {
        return instance->start_task(priority, stack_depth);
    }
    return ESP_FAIL;
}

float dht11_get_temperature() {
    DHT11Sensor* instance = DHT11Sensor::get_instance();
    if (instance) {
        return instance->get_temperature();
    }
    return NAN;
}

float dht11_get_humidity() {
    DHT11Sensor* instance = DHT11Sensor::get_instance();
    if (instance) {
        return instance->get_humidity();
    }
    return NAN;
}

void dht11_notify_read() {
    DHT11Sensor* instance = DHT11Sensor::get_instance();
    if (instance) {
        instance->notify_read();
    } else {
        ESP_LOGE(TAG, "DHT11 TASK HANDLE IS NULL, CAN'T SEND NOTIF");
    }
}

void dht11_get_history(dht11_reading_t* history_buffer, uint32_t* num_readings) {
    DHT11Sensor* instance = DHT11Sensor::get_instance();
    if (instance) {
        instance->get_history(history_buffer, num_readings);
    } else {
        *num_readings = 0;
    }
}

uint64_t dht11_get_last_read() {
    DHT11Sensor* instance = DHT11Sensor::get_instance();
    if (instance) {
        return instance->get_last_read();
    }
    return 0;
}

void DHT11Sensor::read_data_task_wrapper(void* pvParameters) {
    DHT11Sensor* instance = static_cast<DHT11Sensor*>(pvParameters);
    if (instance) {
        instance->read_data_loop();
    }
    vTaskDelete(nullptr);
}

void DHT11Sensor::read_data_loop() {
    ESP_LOGI(TAG, "DHT11 reading task started");

    float temp_c                    = 0.0f;
    float hum_c                     = 0.0f;
    uint64_t last_read_attempt_time = 0.0f;

    read_dht_data(&temp_c, &hum_c, true);
    last_read_attempt_time = esp_timer_get_time();
    ESP_LOGI(TAG, "Dummy reading taken");
    vTaskDelay(pdMS_TO_TICKS(DHT11_COOLDOWN));

    while (true) {
        esp_err_t ret;
        uint64_t current_time_us      = esp_timer_get_time();
        uint64_t time_since_last_read = current_time_us - last_read_attempt_time;

        if (time_since_last_read < MIN_READ_INTERVAL_US) {
            uint64_t remaining_wait = MIN_READ_INTERVAL_US - time_since_last_read;
            xTaskNotifyWait(0, 0, nullptr, pdMS_TO_TICKS(remaining_wait / 1000));
            continue;
        }

        last_read_attempt_time = esp_timer_get_time();

        for (int attempts = 1; attempts <= MAXATTEMPTS; attempts++) {
            bool suppress_driver_logs = (attempts < MAXATTEMPTS);

            ret = read_dht_data(&temp_c, &hum_c, suppress_driver_logs);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "DHT11 read attempt failed, retrying (%d/%d)", attempts, MAXATTEMPTS);
                vTaskDelay(pdMS_TO_TICKS(DHT11_COOLDOWN));
            } else {
                break;
            }
        }

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "CRITICAL ERROR, FAILED TO READ DHT11 DATA");
        } else {
            if (xSemaphoreTake(this->mutex, portMAX_DELAY) == pdTRUE) {
                this->temperature = temp_c * (9.0 / 5.0) + 32;
                this->humidity    = hum_c;

                this->dht_history[this->history_idx].temperature = this->temperature;
                this->dht_history[this->history_idx].humidity    = this->humidity;
                this->dht_history[this->history_idx].timestamp   = time(NULL);
                this->history_idx++;
                if (this->history_idx == DHT_HISTORY_SIZE) {
                    this->history_idx = 0;
                }
                if (this->num_history_readings < DHT_HISTORY_SIZE) {
                    this->num_history_readings++;
                }
                this->last_successful_read = esp_timer_get_time();

                xSemaphoreGive(this->mutex);

                Speaker::get_instance() -> play_sound();
                LCDDisplay::get_instance() -> notify_new_data();
            } else {
                ESP_LOGE(TAG, "ERROR: dht11 read task failed to take mutex");
            }

            ESP_LOGI(TAG, "Temperature: %.2f F, Humidity: %.1f %%", this->temperature, this->humidity);
        }
        xTaskNotifyWait(0, 0, nullptr, pdMS_TO_TICKS(60000));
    }
}