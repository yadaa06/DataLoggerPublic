// timeset.h

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

#define TIME_SYNC_SUCCESS_BIT BIT0

enum {
    TIME_SYNC_COMPLETE,
};

esp_err_t timeset_driver_start_and_wait(void);

#ifdef __cplusplus
}
#endif