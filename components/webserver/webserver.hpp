// webserver.hpp

#pragma once

#include "esp_err.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
#include <string>
#include <vector>

class Webserver {
  private:
    static Webserver* s_webserver_instance;
    static SemaphoreHandle_t s_clients_mutex;
    httpd_handle_t server = nullptr;

    static std::vector<int> s_connected_clients;

    static esp_err_t dht_history_get_handler(httpd_req_t* req);
    static esp_err_t dht_data_get_handler(httpd_req_t* req);
    static esp_err_t root_get_handler(httpd_req_t* req);
    static esp_err_t style_css_get_handler(httpd_req_t* req);
    static esp_err_t script_js_get_handler(httpd_req_t* req);
    static esp_err_t lcd_toggle_handler(httpd_req_t* req);
    static esp_err_t speaker_toggle_handler(httpd_req_t* req);
    static esp_err_t status_get_handler(httpd_req_t* req);
    static esp_err_t websocket_handler(httpd_req_t* req);
    static httpd_handle_t s_websocket_handle;

  public:
    Webserver();
    ~Webserver();
    static Webserver* get_instance();
    void send_state_update(const std::string& state_json);

    esp_err_t start();
    void stop();
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t start_webserver();

#ifdef __cplusplus
}
#endif