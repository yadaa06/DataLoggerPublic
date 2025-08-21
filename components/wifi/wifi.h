// wifi.h

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define WIFI_MAX_SSID_LEN       32
#define WIFI_MAX_PSWD_LEN       64
#define WIFI_CONNECTED_BIT      BIT0
#define WIFI_FAIL_BIT           BIT1
#define MAX_RETRY               10

esp_err_t wifi_driver_start_and_connect_and_wait(const char* ssid, const char* pswd);

#ifdef __cplusplus
}
#endif