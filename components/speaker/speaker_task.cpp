// speaker_task.cpp

#include "speaker_task.hpp"
#include "webserver.hpp"
#include "power_on.h"
#include "power_off.h"
#include "reading_taken.h"
#include "esp_log.h"

static const char* TAG = "SPEAKER_TASK";

Speaker* Speaker::s_speaker_instance = nullptr;

Speaker::Speaker() {}

Speaker::~Speaker() {
    if (task_handle) {
        vTaskDelete(task_handle);
    }
    speaker_driver_deinit();
}

Speaker* Speaker::get_instance() {
    if (!s_speaker_instance) {
        s_speaker_instance = new Speaker();
    }
    return s_speaker_instance;
}

void Speaker::play_sound() {
    if (task_handle) {
        xTaskNotify(task_handle, SPEAKER_PLAY_SOUND, eSetValueWithOverwrite);
    }
}

void Speaker::toggle_power() {
    if (task_handle) {
        xTaskNotify(task_handle, SPEAKER_POWER_TOGGLE, eSetValueWithOverwrite);
        is_speaker_on ^= 1;
        char json_buffer[64];
        snprintf(json_buffer, sizeof(json_buffer), "{\"speaker_on\": %s}", is_speaker_on ? "true" : "false");
        Webserver::get_instance()->send_state_update(json_buffer);
    }
}

bool Speaker::is_on() {
    return is_speaker_on;
}

esp_err_t Speaker::start_task(BaseType_t priority, uint32_t stack_depth) {
    BaseType_t task_result = xTaskCreate(
        Speaker::speaker_task_wrapper,
        "speaker_task",
        stack_depth,
        this,
        priority,
        &task_handle
    );
    if (task_result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create speaker task");
        return ESP_FAIL;
    }
    return ESP_OK;
}

void Speaker::speaker_task_wrapper(void* pvParameters) {
    Speaker* instance = static_cast<Speaker*>(pvParameters);
    if (instance) {
        instance->speaker_task_loop();
    }
    vTaskDelete(nullptr);
}

void Speaker::speaker_task_loop() {
    ESP_LOGI(TAG, "Speaker task started");
    while (true) {
        uint32_t notified_value;
        BaseType_t result = xTaskNotifyWait(0, UINT32_MAX, &notified_value, portMAX_DELAY);

        if (result == pdTRUE) {
            if (notified_value == SPEAKER_POWER_TOGGLE) {
                if (is_speaker_on) {
                    speaker_driver_deinit();
                    speaker_driver_play_sound(power_off_fx, power_off_len);
                    ESP_LOGI(TAG, "Speaker turned OFF");
                } else {
                    speaker_driver_init();
                    speaker_driver_play_sound(power_on_fx, power_on_len);
                    ESP_LOGI(TAG, "Speaker turned ON");
                }
            } else if (notified_value == SPEAKER_PLAY_SOUND && is_speaker_on) {
                ESP_LOGI(TAG, "Playing sound");
                speaker_driver_play_sound(reading_taken_fx, reading_taken_len);
            }
        }
    }
}

void speaker_play_sound(void) {
    Speaker* speaker = Speaker::get_instance();
    if (speaker) {
        speaker->play_sound();
    }
}

esp_err_t start_speaker_task(BaseType_t priority, uint32_t stack_depth) {
    Speaker* speaker = Speaker::get_instance();
    if (speaker) {
        return speaker->start_task(priority, stack_depth);
    }
    return ESP_FAIL;
}