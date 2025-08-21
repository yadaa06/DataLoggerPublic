// speaker.c

#include "speaker.h"
#include "driver/dac_continuous.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2S_PORT I2S_NUM_0

static const char* TAG = "SPEAKER_DRIVER";
static dac_continuous_handle_t dac_handle;

esp_err_t speaker_driver_init(void) {
    if (dac_handle) {
        return ESP_OK;
    }

    dac_continuous_config_t dac_cfg = {
        .chan_mask = DAC_CHANNEL_MASK_CH0,
        .desc_num  = 4,
        .buf_size  = 1024,
        .freq_hz   = 16000,
        .offset    = 0,
        .clk_src   = DAC_DIGI_CLK_SRC_APLL,
    };

    ESP_ERROR_CHECK(dac_continuous_new_channels(&dac_cfg, &dac_handle));
    ESP_ERROR_CHECK(dac_continuous_enable(dac_handle));
    return ESP_OK;
}

esp_err_t speaker_driver_deinit(void) {
    if (dac_handle) {
        return ESP_OK;
    }
    dac_continuous_disable(dac_handle);
    dac_continuous_del_channels(dac_handle);
    dac_handle = NULL;
    return ESP_OK;
}

esp_err_t speaker_driver_play_sound(uint8_t* audio_data, size_t data_len) {
    if (!dac_handle) {
        ESP_LOGE(TAG, "DAC NOT INITIALIZED");
        return ESP_FAIL;
    }
    size_t written = 0;
    ESP_ERROR_CHECK(dac_continuous_write(dac_handle, audio_data, data_len, &written, portMAX_DELAY));
    return ESP_OK;
}