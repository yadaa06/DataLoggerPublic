// webserver.cpp

#include "webserver.hpp"
#include "dht11_task.hpp"
#include "lcd_task.hpp"
#include "speaker_task.hpp"
#include "esp_log.h"
#include <freertos/task.h>
#include <math.h>
#include <string>

static const char* TAG = "WEB_SERVER";

Webserver* Webserver::s_webserver_instance = nullptr;
SemaphoreHandle_t Webserver::s_clients_mutex = nullptr;
httpd_handle_t Webserver::s_websocket_handle = nullptr;
std::vector<int> Webserver::s_connected_clients;

extern const uint8_t _binary_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t _binary_index_html_end[] asm("_binary_index_html_end");
extern const uint8_t _binary_style_css_start[] asm("_binary_style_css_start");
extern const uint8_t _binary_style_css_end[] asm("_binary_style_css_end");
extern const uint8_t _binary_script_js_start[] asm("_binary_script_js_start");
extern const uint8_t _binary_script_js_end[] asm("_binary_script_js_end");

Webserver::Webserver() {
    if (s_clients_mutex == nullptr) {
        s_clients_mutex = xSemaphoreCreateMutex();
    }
}

Webserver::~Webserver() {
    stop();
}

Webserver* Webserver::get_instance() {
    if (s_webserver_instance == nullptr) {
        s_webserver_instance = new Webserver();
    }
    return s_webserver_instance;
}

esp_err_t Webserver::start() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 12;
    esp_err_t result = httpd_start(&server, &config);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Server start failed with error: %s", esp_err_to_name(result));
        return result;
    }

    httpd_uri_t root_uri = {
        .uri                      = "/",
        .method                   = HTTP_GET,
        .handler                  = root_get_handler,
        .user_ctx                 = this,
        .is_websocket             = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol    = NULL};

    httpd_register_uri_handler(server, &root_uri);

    httpd_uri_t style_css_uri = {
        .uri                      = "/style.css",
        .method                   = HTTP_GET,
        .handler                  = style_css_get_handler,
        .user_ctx                 = this,
        .is_websocket             = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol    = NULL};
    httpd_register_uri_handler(server, &style_css_uri);

    httpd_uri_t script_js_uri = {
        .uri                      = "/script.js",
        .method                   = HTTP_GET,
        .handler                  = script_js_get_handler,
        .user_ctx                 = this,
        .is_websocket             = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol    = NULL};
    httpd_register_uri_handler(server, &script_js_uri);

    httpd_uri_t dht_data_uri = {
        .uri                      = "/dht_data",
        .method                   = HTTP_GET,
        .handler                  = dht_data_get_handler,
        .user_ctx                 = this,
        .is_websocket             = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol    = NULL};
    httpd_register_uri_handler(server, &dht_data_uri);

    httpd_uri_t dht_history_uri = {
        .uri                      = "/dht_history",
        .method                   = HTTP_GET,
        .handler                  = dht_history_get_handler,
        .user_ctx                 = this,
        .is_websocket             = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol    = NULL};
    httpd_register_uri_handler(server, &dht_history_uri);

    httpd_uri_t lcd_toggle_uri = {
        .uri = "/lcd_toggle",
        .method = HTTP_POST, 
        .handler = lcd_toggle_handler,
        .user_ctx                 = this,
        .is_websocket             = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol    = NULL};
    httpd_register_uri_handler(server, &lcd_toggle_uri);

    httpd_uri_t speaker_toggle_uri = {
        .uri = "/speaker_toggle",
        .method = HTTP_POST,
        .handler = speaker_toggle_handler,
        .user_ctx                 = this,
        .is_websocket             = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol    = NULL};
    httpd_register_uri_handler(server, &speaker_toggle_uri);

    httpd_uri_t status_uri = {
        .uri = "/status",
        .method = HTTP_GET,
        .handler = status_get_handler,
        .user_ctx = this,
        .is_websocket = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL};
    httpd_register_uri_handler(server, &status_uri);

    httpd_uri_t websocket_uri = {
        .uri                      = "/ws",
        .method                   = HTTP_GET,
        .handler                  = websocket_handler,
        .user_ctx                 = this,
        .is_websocket             = true,
        .handle_ws_control_frames = false,
        .supported_subprotocol    = NULL};
    httpd_register_uri_handler(server, &websocket_uri);
    s_websocket_handle = server;

    ESP_LOGI(TAG, "Server start successful");

    return ESP_OK;
}

void Webserver::stop() {
    if (server) {
        httpd_stop(server);
        server = nullptr;
        ESP_LOGI(TAG, "Server stopped");
        xSemaphoreTake(s_clients_mutex, portMAX_DELAY);
        s_connected_clients.clear();
        xSemaphoreGive(s_clients_mutex);
    }
}

esp_err_t Webserver::dht_history_get_handler(httpd_req_t* req) {
    DHT11Sensor* dht_sensor = DHT11Sensor::get_instance();

    if (!dht_sensor) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "DHT11 sensor not available");
        return ESP_FAIL;
    }

    dht11_reading_t history_buffer[DHT_HISTORY_SIZE];
    uint32_t num_readings = 0;
    dht_sensor->get_history(history_buffer, &num_readings);

    std::string json_response = "{\"history\":[";
    for (uint32_t i = 0; i < num_readings; ++i) {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "{\"temperature\":%.2f,\"humidity\":%.1f,\"timestamp\":%lld}",
                 history_buffer[i].temperature, history_buffer[i].humidity, (long long)history_buffer[i].timestamp);
        json_response += buffer;
        if (i < num_readings - 1) {
            json_response += ",";
        }
    }
    json_response += "]}";

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response.c_str(), json_response.length());
    ESP_LOGI(TAG, "Sent history data.");
    return ESP_OK;
}

esp_err_t Webserver::dht_data_get_handler(httpd_req_t* req) {
    DHT11Sensor* dhtSensor = DHT11Sensor::get_instance();
    if (!dhtSensor) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "DHT11 sensor not available");
        return ESP_FAIL;
    }

    dhtSensor->notify_read();

    float temperature = dhtSensor->get_temperature();
    float humidity    = dhtSensor->get_humidity();

    std::string json_response;
    if (isnan(temperature) || isnan(humidity)) {
        json_response = "{\"temperature\": null, \"humidity\": null}";
    } else {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "{\"temperature\": %.2f, \"humidity\": %.1f}", temperature, humidity);
        json_response = buffer;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response.c_str(), json_response.length());
    ESP_LOGI(TAG, "Sent DHT data: %s", json_response.c_str());
    return ESP_OK;
}

esp_err_t Webserver::root_get_handler(httpd_req_t* req) {
    ESP_LOGI(TAG, "Serving root page");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char*)_binary_index_html_start, _binary_index_html_end - _binary_index_html_start);
    return ESP_OK;
}

esp_err_t Webserver::style_css_get_handler(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char*)_binary_style_css_start, _binary_style_css_end - _binary_style_css_start);
    return ESP_OK;
}

esp_err_t Webserver::script_js_get_handler(httpd_req_t* req) {
    ESP_LOGI(TAG, "Serving script.js");
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char*)_binary_script_js_start, _binary_script_js_end - _binary_script_js_start);
    return ESP_OK;
}

esp_err_t Webserver::lcd_toggle_handler(httpd_req_t* req) {
    LCDDisplay::get_instance()->toggle_power();

    vTaskDelay(pdMS_TO_TICKS(200));

    Webserver::get_instance()->send_state_update(
        "{\"lcd_on\": " + std::string(LCDDisplay::get_instance()->is_on() ? "true" : "false") +
        ", \"speaker_on\": " + std::string(Speaker::get_instance()->is_on() ? "true" : "false") +
        "}"
    );

    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t Webserver::speaker_toggle_handler(httpd_req_t* req) {
    Speaker::get_instance()->toggle_power();

    Webserver::get_instance()->send_state_update(
        "{\"lcd_on\": " + std::string(LCDDisplay::get_instance()->is_on() ? "true" : "false") +
        ", \"speaker_on\": " + std::string(Speaker::get_instance()->is_on() ? "true" : "false") +
        "}"
    );

    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t Webserver::status_get_handler(httpd_req_t* req) {
    LCDDisplay* lcd = LCDDisplay::get_instance();
    Speaker* speaker = Speaker::get_instance();

    char json_string[128];
    int len = snprintf(json_string, sizeof(json_string), "{\"lcd_on\": %s, \"speaker_on\": %s}",
                       lcd && lcd->is_on() ? "true" : "false",
                       speaker && speaker->is_on() ? "true" : "false");

    httpd_resp_set_type(req, "application/json");
    if (len > 0 && len < sizeof(json_string)) {
        httpd_resp_send(req, json_string, len);
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to format JSON data");
    }

    return ESP_OK;
}

esp_err_t Webserver::websocket_handler(httpd_req_t* req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI("WEB_SERVER", "WebSocket handshake done, client connected");

        int sockfd = httpd_req_to_sockfd(req);
        if (sockfd >= 0) {
            xSemaphoreTake(s_clients_mutex, portMAX_DELAY);
            s_connected_clients.push_back(sockfd);
            xSemaphoreGive(s_clients_mutex);
        }
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE("WEB_SERVER", "Failed to get WS frame length: %s", esp_err_to_name(ret));
        return ret;
    }

    if (ws_pkt.len > 0) {
        uint8_t* buf = (uint8_t*)malloc(ws_pkt.len + 1);
        if (!buf) {
            return ESP_ERR_NO_MEM;
        }

        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret == ESP_OK) {
            buf[ws_pkt.len] = '\0';
            ESP_LOGI("WEB_SERVER", "Received WS message: %s", buf);
        }
        free(buf);
    }

    return ESP_OK;
}

void Webserver::send_state_update(const std::string& state_json) {
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t*)state_json.c_str();
    ws_pkt.len = state_json.length();
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    xSemaphoreTake(s_clients_mutex, portMAX_DELAY);

    auto it = s_connected_clients.begin();
    while (it != s_connected_clients.end()) {
        int sockfd = *it;
        esp_err_t ret = httpd_ws_send_frame_async(s_websocket_handle, sockfd, &ws_pkt);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Removing disconnected client (sock %d), err: %s", sockfd, esp_err_to_name(ret));
            it = s_connected_clients.erase(it);
        } else {
            ++it;
        }
    }

    xSemaphoreGive(s_clients_mutex);
}

esp_err_t start_webserver() {
    Webserver* server = Webserver::get_instance();
    if (server) {
        return server->start();
    }
    return ESP_FAIL;
}