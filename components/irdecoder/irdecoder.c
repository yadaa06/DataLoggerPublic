// irdecoder.c

#include "irdecoder.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "rom/ets_sys.h"
#include <string.h>

static const char* TAG = "IR_DRIVER";

static uint64_t last_time = 0;

static volatile uint8_t active_idx = 0;
static volatile uint8_t decode_len = 0;

static volatile uint32_t ir_buffer_A[IR_TIMES_SIZE];
static volatile uint32_t ir_buffer_B[IR_TIMES_SIZE];

static volatile uint32_t* a_active_buffer = ir_buffer_A;
static volatile uint32_t* a_decode_buffer = ir_buffer_B;

SemaphoreHandle_t xDecoderSignaler = NULL;
static TimerHandle_t ir_timeout_timer = NULL;

gptimer_handle_t GPtimer;

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint64_t curr_time, pulse_length;
    gptimer_get_raw_count(GPtimer, &curr_time);

    if (last_time == 0) {
        last_time = curr_time;
        return;
    }

    pulse_length = curr_time - last_time;

    BaseType_t higher_priority_task = pdFALSE;
    xTimerResetFromISR(ir_timeout_timer, &higher_priority_task);

    last_time = curr_time;

    if (pulse_length > 15000) {
        if (active_idx > 0) {
            volatile uint32_t* tmp = a_active_buffer;

            a_active_buffer = a_decode_buffer;
            a_decode_buffer = tmp;
            decode_len      = active_idx;

            xSemaphoreGiveFromISR(xDecoderSignaler, &higher_priority_task);
        }
        active_idx = 0;
    } else {
        if (active_idx < IR_TIMES_SIZE) {
            a_active_buffer[active_idx++] = pulse_length;
        } else {
            last_time  = 0;
            active_idx = 0;
        }
    }
}

static void ir_timeout_callback(TimerHandle_t xTimer) {
    volatile uint32_t* tmp = a_active_buffer;

    a_active_buffer = a_decode_buffer;
    a_decode_buffer = tmp;

    decode_len = active_idx;
    active_idx = 0;
    last_time  = 0;

    BaseType_t higher_priority_task = pdFALSE;
    xSemaphoreGiveFromISR(xDecoderSignaler, &higher_priority_task);
}

esp_err_t ir_decoder_init() {
    gpio_reset_pin(IR_PIN);
    gpio_set_direction(IR_PIN, GPIO_MODE_INPUT);
    gpio_set_intr_type(IR_PIN, GPIO_INTR_ANYEDGE);

    gptimer_config_t timer_config = {
        .clk_src       = GPTIMER_CLK_SRC_DEFAULT,
        .direction     = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000};

    xDecoderSignaler = xSemaphoreCreateBinary();
    if (xDecoderSignaler == NULL) {
        ESP_LOGE(TAG, "FAILED TO CREATE SEMAPHORE");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &GPtimer));
    esp_err_t isr_service_result = gpio_install_isr_service(0);
    if (isr_service_result != ESP_OK && isr_service_result != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to install ISR service: %s", esp_err_to_name(isr_service_result));
    }
    ESP_ERROR_CHECK(gpio_isr_handler_add(IR_PIN, gpio_isr_handler, NULL));

    ir_timeout_timer = xTimerCreate(
        "IRTimeout",
        pdMS_TO_TICKS(TIMEOUT_US / 1000),
        pdFALSE,
        NULL,
        ir_timeout_callback);

    if (ir_timeout_timer == NULL) {
        ESP_LOGE(TAG, "FAILED TO CREATE SOFTWARE TIMER");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(gptimer_enable(GPtimer));
    ESP_ERROR_CHECK(gptimer_start(GPtimer));
    
    return ESP_OK;
}

static void ir_decode(ir_result_t* result) {
    result->type = IR_FRAME_TYPE_INVALID;

    result->address = 0;
    result->command = 0;

    if (decode_len > 50) {
        if (!IR_MATCH(a_decode_buffer[0], NEC_START_PULSE) || !IR_MATCH(a_decode_buffer[1], NEC_START_SPACE)) {
            ESP_LOGE(TAG, "IR SIGNAL DOESN'T FOLLOW NEC START PROTOCOL");
            return;
        }

        uint32_t decoded_data = 0;
        for (int i = 0; i < 32; i++) {
            uint32_t pulse = a_decode_buffer[i * 2 + 2];
            uint32_t space = a_decode_buffer[i * 2 + 3];

            if (!IR_MATCH(pulse, NEC_BIT_PULSE)) {
                ESP_LOGE(TAG, "IR BIT NOT A VALID BIT PULSE");
                return;
            }

            decoded_data <<= 1;
            if (IR_MATCH(space, NEC_ONE_SPACE)) {
                decoded_data |= 1;
            } else if (IR_MATCH(space, NEC_ZERO_SPACE)) {
                decoded_data |= 0;
            } else {
                ESP_LOGE(TAG, "IR BIT NOT A VALID BIT SPACE");
                return;
            }
        }

        uint8_t addr     = (decoded_data >> 24) & 0xFF;
        uint8_t inv_addr = (decoded_data >> 16) & 0xFF;
        uint8_t cmd      = (decoded_data >> 8) & 0xFF;
        uint8_t inv_cmd  = (decoded_data) & 0xFF;

        if ((addr ^ inv_addr) != 0xFF || (cmd ^ inv_cmd) != 0xFF) {
            ESP_LOGE(TAG, "CHECKSUM FAILED");
            return;
        }

        result->type = IR_FRAME_TYPE_DATA;

        result->address = addr;
        result->command = cmd;

        return;
    }

    else if (decode_len > 0 && decode_len < 10) {
        result->type = IR_FRAME_TYPE_REPEAT;
        return;
    }
}

void ir_decoder_get_data(ir_result_t* result) {
    ir_decode(result);
}