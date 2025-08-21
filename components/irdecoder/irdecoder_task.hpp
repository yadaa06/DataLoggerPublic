// irdecoder_task.hpp

#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "irdecoder.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BUTTON_0                = 0x68,
    BUTTON_1                = 0x30,
    BUTTON_2                = 0x18,
    BUTTON_3                = 0x7A,
    BUTTON_4                = 0x10,
    BUTTON_5                = 0x38,
    BUTTON_6                = 0x5A,
    BUTTON_7                = 0x42,
    BUTTON_8                = 0x4A,
    BUTTON_9                = 0x52,
    BUTTON_PLUS             = 0x90,
    BUTTON_MINUS            = 0xA8,
    BUTTON_EQ               = 0xE0,
    BUTTON_U_SD             = 0xB0,
    BUTTON_CYCLE            = 0x98,
    BUTTON_PLAY_PAUSE       = 0x22,
    BUTTON_BACKWARD         = 0x02,
    BUTTON_FORWARD          = 0xC2,
    BUTTON_POWER            = 0xA2,
    BUTTON_MUTE             = 0xE2,
    BUTTON_MODE             = 0x62,
    BUTTON_UNKNOWN_OR_ERROR = 0xFF
} button_press_t;

const char* get_button_name(button_press_t button);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <map>

class IRDecoder {
  private:
    static IRDecoder* s_decoder_instance;
    TaskHandle_t task_handle = nullptr;

    static const std::map<uint8_t, button_press_t> command_map;

    void map_command_to_action(uint8_t cmd);
    static void decoder_task_wrapper(void* pvParameters);
    void decoder_task_loop();

  public:
    IRDecoder();
    ~IRDecoder();
    static IRDecoder* get_instance();

    esp_err_t start_task(BaseType_t priority, uint32_t stack_depth);
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t start_ir_decoder_task(BaseType_t priority, uint32_t stack_depth);

#ifdef __cplusplus
}
#endif