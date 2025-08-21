// irdecoder.h

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t xDecoderSignaler;

#ifdef __cplusplus
extern "C" {
#endif

#define IR_PIN GPIO_NUM_14
#define IR_TIMES_SIZE 128

#define TIMEOUT_US 100000

#define NEC_START_PULSE 9000
#define NEC_START_SPACE 4500
#define NEC_BIT_PULSE 560
#define NEC_ZERO_SPACE 560
#define NEC_ONE_SPACE 1690
#define NEC_TOLERANCE_PERCENT 30

#define IR_MATCH(value, target)                                          \
    ((value) >= ((target) - ((target) * NEC_TOLERANCE_PERCENT / 100)) && \
     (value) <= ((target) + ((target) * NEC_TOLERANCE_PERCENT / 100)))

typedef enum {
    IR_FRAME_TYPE_DATA,
    IR_FRAME_TYPE_REPEAT,
    IR_FRAME_TYPE_INVALID,
} ir_frame_type_t;

typedef struct {
    ir_frame_type_t type;
    uint8_t address;
    uint8_t command;
} ir_result_t;

esp_err_t ir_decoder_init();
bool ir_decoder_has_data();
void ir_decoder_get_data(ir_result_t* result);

#ifdef __cplusplus
}
#endif