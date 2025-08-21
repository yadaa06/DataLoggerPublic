// dht11_task.hpp

#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "time.h"

#define DHT11_COOLDOWN 3000
#define MAXATTEMPTS 3
#define MIN_READ_INTERVAL_US 3000000
#define DHT_HISTORY_SIZE 60

typedef struct {
    float temperature;
    float humidity;
    time_t timestamp;
} dht11_reading_t;

#ifdef __cplusplus
#include <cmath>

class DHT11Sensor {
  private:
    static DHT11Sensor* s_dht11_instance;
    TaskHandle_t task_handle     = nullptr;

    SemaphoreHandle_t mutex = nullptr;
    dht11_reading_t dht_history[DHT_HISTORY_SIZE];
    float temperature             = NAN;
    float humidity                = NAN;
    int history_idx               = 0;
    int num_history_readings      = 0;
    uint64_t last_successful_read = 0;

    static void read_data_task_wrapper(void* pvParameters);
    void read_data_loop();

  public:
    DHT11Sensor();
    ~DHT11Sensor();
    static DHT11Sensor* get_instance();

    esp_err_t start_task(BaseType_t priority, uint32_t stack_depth);
    void notify_read();
    float get_temperature();
    float get_humidity();
    void get_history(dht11_reading_t* history_buffer, uint32_t* num_readings);
    uint64_t get_last_read();
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t start_dht11_sensor_task(BaseType_t priority, uint32_t stack_depth);
float dht11_get_temperature();
float dht11_get_humidity();
void dht11_notify_read();
void dht11_get_history(dht11_reading_t* history_buffer, uint32_t* num_readings);
uint64_t dht11_get_last_read();

#ifdef __cplusplus
}
#endif