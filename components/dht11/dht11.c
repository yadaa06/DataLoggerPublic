// dht11.c

#include "dht11.h"     
#include <stdbool.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"

static const char* TAG = "DHT11_DRIVER";

esp_err_t read_dht_data(float* temperature, float* humidity, bool suppressLogErrors) {
    uint8_t data[5] = {0, 0, 0, 0, 0};
    esp_err_t ret   = ESP_OK;

    // 1. Send start signal
    gpio_set_direction(DHT11_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_PIN, 0);
    esp_rom_delay_us(20000);
    gpio_set_level(DHT11_PIN, 1);
    esp_rom_delay_us(40);
    gpio_set_direction(DHT11_PIN, GPIO_MODE_INPUT);

    // 2. DHT Response
    uint64_t start_time = esp_timer_get_time();

    while (gpio_get_level(DHT11_PIN) == 1) {
        if (esp_timer_get_time() - start_time > 100) {
            ret = ESP_FAIL;
            goto exit_critical;
        }
    }

    start_time = esp_timer_get_time();
    while (gpio_get_level(DHT11_PIN) == 0) {
        if (esp_timer_get_time() - start_time > 100) {
            ret = ESP_FAIL;
            goto exit_critical;
        }
    }

    start_time = esp_timer_get_time();
    while (gpio_get_level(DHT11_PIN) == 1) {
        if (esp_timer_get_time() - start_time > 100) {
            ret = ESP_FAIL;
            goto exit_critical;
        }
    }

    // 3. Data Transmission
    for (uint8_t i = 0; i < 5; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            start_time = esp_timer_get_time();
            while (gpio_get_level(DHT11_PIN) == 0) {
                if (esp_timer_get_time() - start_time > 70) {
                    ret = ESP_FAIL;
                    goto exit_critical;
                }
            }
            start_time = esp_timer_get_time();
            while (gpio_get_level(DHT11_PIN) == 1) {
                if (esp_timer_get_time() - start_time > 120) {
                    ret = ESP_FAIL;
                    goto exit_critical;
                }
            }

            uint64_t pulse_duration = esp_timer_get_time() - start_time;
            data[i] <<= 1;
            if (pulse_duration > 40) {
                data[i] |= 1;
            }
        }
    }

    // 4. Checksum
    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        ret = ESP_ERR_INVALID_CRC;
    } else {
        *humidity    = (float)data[0] + (float)data[1] / 10.0f;
        *temperature = (float)data[2] + (float)data[3] / 10.0f;

        ret = ESP_OK;
    }

exit_critical:

    if (ret != ESP_OK && !suppressLogErrors) {
        if (ret == ESP_ERR_INVALID_CRC) {
            ESP_LOGE(TAG, "CHECKSUM FAILED");
            return ESP_FAIL;
        } else {
            ESP_LOGE(TAG, "DHT timing error: %s", esp_err_to_name(ret));
            return ESP_FAIL;
        }
    } else {
        ESP_LOGI(TAG, "This round of data is VALID");
        return ESP_OK;
    }

    return ESP_OK;
}