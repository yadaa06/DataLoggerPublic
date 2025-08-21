// irdecoder_task.cpp

#include "irdecoder_task.hpp"
#include "dht11_task.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_task.hpp"
#include "speaker_task.hpp"
#include <map>

static const char* TAG = "IR_DECODER_TASK";

IRDecoder* IRDecoder::s_decoder_instance = nullptr;

const std::map<uint8_t, button_press_t> IRDecoder::command_map = {
    {0x68, BUTTON_0},
    {0x30, BUTTON_1},
    {0x18, BUTTON_2},
    {0x7A, BUTTON_3},
    {0x10, BUTTON_4},
    {0x38, BUTTON_5},
    {0x5A, BUTTON_6},
    {0x42, BUTTON_7},
    {0x4A, BUTTON_8},
    {0x52, BUTTON_9},
    {0x90, BUTTON_PLUS},
    {0xA8, BUTTON_MINUS},
    {0xE0, BUTTON_EQ},
    {0xB0, BUTTON_U_SD},
    {0x98, BUTTON_CYCLE},
    {0x22, BUTTON_PLAY_PAUSE},
    {0x02, BUTTON_BACKWARD},
    {0xC2, BUTTON_FORWARD},
    {0xA2, BUTTON_POWER},
    {0xE2, BUTTON_MUTE},
    {0x62, BUTTON_MODE}};

IRDecoder::IRDecoder() {
}

IRDecoder::~IRDecoder() {
    if (task_handle) {
        vTaskDelete(task_handle);
    }
}

IRDecoder* IRDecoder::get_instance() {
    if (!s_decoder_instance) {
        s_decoder_instance = new IRDecoder();
    }

    return s_decoder_instance;
}

void IRDecoder::map_command_to_action(uint8_t cmd) {
    std::map<uint8_t, button_press_t>::const_iterator it = command_map.find(cmd);

    if (it != command_map.end()) {
        button_press_t button = it->second;
        switch (button) {
        case BUTTON_FORWARD:
            DHT11Sensor::get_instance()->notify_read();
            ESP_LOGI(TAG, "Button: FORWARD");
            break;
        case BUTTON_CYCLE:
            LCDDisplay::get_instance()->cycle_mode();
            ESP_LOGI(TAG, "Button: CYCLE");
            break;
        case BUTTON_EQ:
            Speaker::get_instance()->play_sound();
            ESP_LOGI(TAG, "Button: EQ");
            break;
        case BUTTON_U_SD:
            LCDDisplay::get_instance()->toggle_power();
            ESP_LOGI(TAG, "Button: U/SD");
            break;
        case BUTTON_MUTE:
            Speaker::get_instance()->toggle_power();
            ESP_LOGI(TAG, "Button: MUTE"); 
            break;
        default:
            ESP_LOGI(TAG, "Button: %s (Command: 0x%02X)", get_button_name(button), cmd);
            break;
        }
    } else {
        ESP_LOGW(TAG, "Unmapped IR command: 0x%02X", cmd);
    }
}

const char* get_button_name(button_press_t button) {
    switch (button) {
    case BUTTON_0:
        return "0";
    case BUTTON_1:
        return "1";
    case BUTTON_2:
        return "2";
    case BUTTON_3:
        return "3";
    case BUTTON_4:
        return "4";
    case BUTTON_5:
        return "5";
    case BUTTON_6:
        return "6";
    case BUTTON_7:
        return "7";
    case BUTTON_8:
        return "8";
    case BUTTON_9:
        return "9";
    case BUTTON_PLUS:
        return "PLUS";
    case BUTTON_MINUS:
        return "MINUS";
    case BUTTON_EQ:
        return "EQ";
    case BUTTON_U_SD:
        return "U/SD";
    case BUTTON_CYCLE:
        return "CYCLE";
    case BUTTON_PLAY_PAUSE:
        return "PLAY/PAUSE";
    case BUTTON_BACKWARD:
        return "BACKWARD";
    case BUTTON_FORWARD:
        return "FORWARD";
    case BUTTON_POWER:
        return "POWER";
    case BUTTON_MUTE:
        return "MUTE";
    case BUTTON_MODE:
        return "MODE";
    case BUTTON_UNKNOWN_OR_ERROR:
        return "UNKNOWN/ERROR";
    default:
        return "UNMAPPED";
    }
}

void IRDecoder::decoder_task_wrapper(void* pvParameters) {
    IRDecoder* instance = static_cast<IRDecoder*>(pvParameters);
    if (instance) {
        instance->decoder_task_loop();
    }
    vTaskDelete(nullptr);
}

esp_err_t IRDecoder::start_task(BaseType_t priority, uint32_t stack_depth) {
    esp_err_t result = ir_decoder_init();
    if (result != ESP_OK) {
        return result;
    }
    BaseType_t task_result = xTaskCreate(
        decoder_task_wrapper,
        "ir_decoder_task",
        stack_depth,
        this,
        priority,
        &this->task_handle);
    if (task_result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create IR decoder task");
        return ESP_FAIL;
    }
    return ESP_OK;
}

void IRDecoder::decoder_task_loop() {
    ESP_LOGI(TAG, "IR decoder task started");
    while (true) {
        if (xSemaphoreTake(xDecoderSignaler, portMAX_DELAY) == pdTRUE) {
            ir_result_t decoded_signal;
            ir_decoder_get_data(&decoded_signal);
            if (decoded_signal.type == IR_FRAME_TYPE_DATA) {
                map_command_to_action(decoded_signal.command);
            } else if (decoded_signal.type == IR_FRAME_TYPE_REPEAT) {
                ESP_LOGI(TAG, "Repeat Code Detected");
            } else {
                ESP_LOGW(TAG, "Invalid Frame Detected");
            }
        }
    }
}

esp_err_t start_ir_decoder_task(BaseType_t priority, uint32_t stack_depth) {
    IRDecoder* decoder = IRDecoder::get_instance();
    if (decoder) {
        return decoder->start_task(priority, stack_depth);
    }
    return ESP_FAIL;
}