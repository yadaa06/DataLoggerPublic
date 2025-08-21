// speaker_task.hpp

#pragma once

#include "speaker.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SPEAKER_PLAY_SOUND 9
#define SPEAKER_POWER_TOGGLE 10

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
class Speaker {
private:
   static Speaker* s_speaker_instance;
   TaskHandle_t task_handle = nullptr; 
    bool is_speaker_on = false;

    static void speaker_task_wrapper(void* pvParameters);
    void speaker_task_loop();
public:
    Speaker();
    ~Speaker();

    static Speaker* get_instance();
    esp_err_t start_task(BaseType_t priority, uint32_t stack_depth);
    void play_sound();
    void toggle_power();
    bool is_on();
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

void speaker_play_sound(void);
esp_err_t start_speaker_task(BaseType_t priority, uint32_t stack_depth);

#ifdef __cplusplus
}
#endif