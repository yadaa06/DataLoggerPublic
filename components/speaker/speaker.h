// speaker.h

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SPEAKER_UL_VALUE 9

esp_err_t speaker_driver_init(void);
esp_err_t speaker_driver_deinit(void);
esp_err_t speaker_driver_play_sound(uint8_t* audio_data, size_t data_len);

#ifdef __cplusplus
}
#endif