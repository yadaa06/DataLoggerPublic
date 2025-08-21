// main.c

#include "button.h"
#include "dht11_task.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "irdecoder_task.hpp"
#include "lcd_task.hpp"
#include "button_task.hpp"
#include "speaker_task.hpp"
#include "statusled.h"
#include "timeset.h"
#include "webserver.hpp"
#include "wifi.h"

#define DHT11_TASK_PRIORITY 15
#define LCD_TASK_PRIORITY 10
#define BUTTON_TASK_PRIORITY 12
#define IR_DECODER_TASK_PRIORITY 9
#define SPEAKER_TASK_PRIORITY 13

static const char* TAG = "APP_MAIN";

TaskHandle_t dht11_task_handle      = NULL;
TaskHandle_t lcd_task_handle        = NULL;
TaskHandle_t button_task_handle     = NULL;
TaskHandle_t ir_decoder_task_handle = NULL;
TaskHandle_t speaker_task_handle    = NULL;

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Application Starting");
    status_led_init();
    status_led_set_state(STATUS_LED_STATE_STARTING);
    
    status_led_set_state(STATUS_LED_STATE_IN_PROGRESS);

    ESP_ERROR_CHECK(wifi_driver_start_and_connect_and_wait("WifiName", "password"));
    timeset_driver_start_and_wait();
    ESP_ERROR_CHECK(LCDDisplay::get_instance() -> start_task(LCD_TASK_PRIORITY, 4096));
    ESP_ERROR_CHECK(DHT11Sensor::get_instance() -> start_task(DHT11_TASK_PRIORITY, 4096));
    ESP_ERROR_CHECK(IRDecoder::get_instance() -> start_task(IR_DECODER_TASK_PRIORITY, 4096));
    ESP_ERROR_CHECK(Button::get_instance() -> start_task(BUTTON_TASK_PRIORITY, 2048));
    ESP_ERROR_CHECK(Speaker::get_instance() -> start_task(SPEAKER_TASK_PRIORITY, 2048));
    ESP_ERROR_CHECK(Webserver::get_instance() -> start());

    status_led_set_state(STATUS_LED_STATE_READY);
}
