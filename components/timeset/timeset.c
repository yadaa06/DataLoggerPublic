// timeset.c

#include "timeset.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <time.h>

static const char* TAG = "TIMESET_DRIVER";

ESP_EVENT_DEFINE_BASE(TIME_SYNC_EVENT);

static void time_sync_event_handler_temp(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == TIME_SYNC_EVENT && event_id == TIME_SYNC_COMPLETE) {
        EventGroupHandle_t temp_event_group = (EventGroupHandle_t)arg;
        xEventGroupSetBits(temp_event_group, BIT0);
    }
}

static void time_sync_notification_cb(struct timeval* tv) {
    ESP_LOGI(TAG, "Time Synchronized fron NTP server");
    esp_event_post(TIME_SYNC_EVENT, TIME_SYNC_COMPLETE, NULL, 0, portMAX_DELAY);
}

static void _setup_time() {
    setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
    tzset();
    ESP_LOGI(TAG, "Timezone set to EST");

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();
    ESP_LOGI(TAG, "Initialized SNTP");
}

esp_err_t timeset_driver_start_and_wait() {
    ESP_LOGI(TAG, "Starting timesync");
    EventGroupHandle_t temp_event_group = xEventGroupCreate();

    if (temp_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create Time Sync Group");
        return ESP_FAIL;
    }
    
    ESP_ERROR_CHECK(esp_event_handler_register(TIME_SYNC_EVENT, TIME_SYNC_COMPLETE, &time_sync_event_handler_temp, temp_event_group));
    
    _setup_time();

    ESP_LOGI(TAG, "Waiting for time synchronization to complete...");
    EventBits_t bits = xEventGroupWaitBits(temp_event_group, TIME_SYNC_SUCCESS_BIT, pdTRUE, pdFALSE, pdMS_TO_TICKS(30000));
    
    esp_event_handler_unregister(TIME_SYNC_EVENT, TIME_SYNC_COMPLETE, &time_sync_event_handler_temp);
    vEventGroupDelete(temp_event_group);

    if (bits & TIME_SYNC_SUCCESS_BIT) {
        ESP_LOGI(TAG, "Time synchronization complete!");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Time synchronization failed.");
        return ESP_FAIL;
    }
}