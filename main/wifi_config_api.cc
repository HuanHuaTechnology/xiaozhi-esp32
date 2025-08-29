#include "wifi_config_api.h"
#include "board.h"
#include "system_info.h"
#include "settings.h"
#include "wifi_station.h"
#include "ssid_manager.h"
#include "application.h"

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_http_server.h>
#include <esp_timer.h>
#include <cstring>

static const char *TAG = "WifiConfigApi";

WifiConfigApi& WifiConfigApi::GetInstance() {
    static WifiConfigApi instance;
    return instance;
}

esp_err_t WifiConfigApi::Init() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port_;
    config.max_uri_handlers = 20;
    config.stack_size = 8192;
    
    ESP_LOGI(TAG, "Starting HTTP server on port %d", port_);
    
    esp_err_t ret = httpd_start(&server_, &config);
    if (ret == ESP_OK) {
        RegisterApiRoutes();
        ESP_LOGI(TAG, "HTTP server started successfully");
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t WifiConfigApi::Start() {
    if (server_ == nullptr) {
        return Init();
    }
    return ESP_OK;
}

void WifiConfigApi::Stop() {
    if (server_ != nullptr) {
        httpd_stop(server_);
        server_ = nullptr;
        ESP_LOGI(TAG, "HTTP server stopped");
    }
}

esp_err_t WifiConfigApi::RegisterApiRoutes() {
    // 根路径 - 返回API信息
    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = ApiRootHandler,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server_, &root_uri);
    
    // WiFi扫描API
    httpd_uri_t wifi_scan_uri = {
        .uri = "/api/wifi/scan",
        .method = HTTP_GET,
        .handler = ApiWifiScanHandler,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server_, &wifi_scan_uri);
    
    // WiFi连接API
    httpd_uri_t wifi_connect_uri = {
        .uri = "/api/wifi/connect",
        .method = HTTP_POST,
        .handler = ApiWifiConnectHandler,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server_, &wifi_connect_uri);
    
    // WiFi状态API
    httpd_uri_t wifi_status_uri = {
        .uri = "/api/wifi/status",
        .method = HTTP_GET,
        .handler = ApiWifiStatusHandler,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server_, &wifi_status_uri);
    
    // WiFi重置API
    httpd_uri_t wifi_reset_uri = {
        .uri = "/api/wifi/reset",
        .method = HTTP_POST,
        .handler = ApiWifiResetHandler,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server_, &wifi_reset_uri);
    
    // 设备信息API
    httpd_uri_t device_info_uri = {
        .uri = "/api/device/info",
        .method = HTTP_GET,
        .handler = ApiDeviceInfoHandler,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server_, &device_info_uri);
    
    // 设备状态API
    httpd_uri_t device_status_uri = {
        .uri = "/api/device/status",
        .method = HTTP_GET,
        .handler = ApiDeviceStatusHandler,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server_, &device_status_uri);
    
    // 设备重启API
    httpd_uri_t device_restart_uri = {
        .uri = "/api/device/restart",
        .method = HTTP_POST,
        .handler = ApiDeviceRestartHandler,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server_, &device_restart_uri);
    
    // 配置API
    httpd_uri_t config_uri = {
        .uri = "/api/config",
        .method = HTTP_GET,
        .handler = ApiConfigHandler,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server_, &config_uri);
    
    // Ping API
    httpd_uri_t ping_uri = {
        .uri = "/api/ping",
        .method = HTTP_GET,
        .handler = ApiPingHandler,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server_, &ping_uri);
    
    return ESP_OK;
}

esp_err_t WifiConfigApi::ApiRootHandler(httpd_req_t *req) {
    const char* response = "{\"name\":\"Xiaozhi WiFi Config API\",\"version\":\"1.0.0\",\"description\":\"WiFi configuration API for Xiaozhi ESP32 devices\"}";
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    return httpd_resp_send(req, response, strlen(response));
}

esp_err_t WifiConfigApi::ApiWifiScanHandler(httpd_req_t *req) {
    ESP_LOGI(TAG, "WiFi scan requested");
    
    // 返回模拟的WiFi网络数据
    const char* response = "{\"success\":true,\"networks\":[{\"ssid\":\"TestWiFi1\",\"rssi\":-45,\"channel\":6,\"encrypted\":true},{\"ssid\":\"TestWiFi2\",\"rssi\":-60,\"channel\":11,\"encrypted\":false}]}";
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    return httpd_resp_send(req, response, strlen(response));
}

esp_err_t WifiConfigApi::ApiWifiConnectHandler(httpd_req_t *req) {
    ESP_LOGI(TAG, "WiFi connect requested");
    
    // 读取请求体
    char content[1024];
    int recv_len = httpd_req_recv(req, content, sizeof(content) - 1);
    if (recv_len <= 0) {
        return httpd_resp_send_500(req);
    }
    content[recv_len] = '\0';
    
    ESP_LOGI(TAG, "WiFi connect request: %s", content);
    
    const char* response = "{\"success\":true,\"message\":\"WiFi connection initiated\"}";
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    return httpd_resp_send(req, response, strlen(response));
}

esp_err_t WifiConfigApi::ApiWifiStatusHandler(httpd_req_t *req) {
    auto& wifi_station = WifiStation::GetInstance();
    
    char response[512];
    snprintf(response, sizeof(response), 
        "{\"success\":true,\"status\":{\"connected\":%s,\"ssid\":\"%s\",\"rssi\":%d,\"channel\":%d,\"ip\":\"%s\"}}",
        wifi_station.IsConnected() ? "true" : "false",
        wifi_station.GetSsid().c_str(),
        wifi_station.GetRssi(),
        wifi_station.GetChannel(),
        wifi_station.GetIpAddress().c_str()
    );
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    return httpd_resp_send(req, response, strlen(response));
}

esp_err_t WifiConfigApi::ApiWifiResetHandler(httpd_req_t *req) {
    ESP_LOGI(TAG, "WiFi reset requested");
    
    const char* response = "{\"success\":true,\"message\":\"WiFi configuration reset\"}";
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    return httpd_resp_send(req, response, strlen(response));
}

esp_err_t WifiConfigApi::ApiDeviceInfoHandler(httpd_req_t *req) {
    auto& board = Board::GetInstance();
    
    char response[512];
    snprintf(response, sizeof(response), 
        "{\"success\":true,\"info\":{\"name\":\"%s\",\"type\":\"%s\",\"mac\":\"%s\",\"uuid\":\"%s\",\"chip_model\":\"%s\"}}",
        BOARD_NAME,
        board.GetBoardType().c_str(),
        SystemInfo::GetMacAddress().c_str(),
        board.GetUuid().c_str(),
        SystemInfo::GetChipModelName().c_str()
    );
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    return httpd_resp_send(req, response, strlen(response));
}

esp_err_t WifiConfigApi::ApiDeviceStatusHandler(httpd_req_t *req) {
    auto& wifi_station = WifiStation::GetInstance();
    
    char response[1024];
    snprintf(response, sizeof(response), 
        "{\"success\":true,\"status\":{\"wifi\":{\"connected\":%s,\"ssid\":\"%s\",\"rssi\":%d,\"ip\":\"%s\"}}}",
        wifi_station.IsConnected() ? "true" : "false",
        wifi_station.GetSsid().c_str(),
        wifi_station.GetRssi(),
        wifi_station.GetIpAddress().c_str()
    );
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    return httpd_resp_send(req, response, strlen(response));
}

esp_err_t WifiConfigApi::ApiDeviceRestartHandler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Device restart requested");
    
    const char* response = "{\"success\":true,\"message\":\"Device will restart in 3 seconds\"}";
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    httpd_resp_send(req, response, strlen(response));
    
    // 延迟重启
    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_restart();
    
    return ESP_OK;
}

esp_err_t WifiConfigApi::ApiConfigHandler(httpd_req_t *req) {
    const char* response = "{\"success\":true,\"message\":\"Configuration API endpoint\"}";
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    return httpd_resp_send(req, response, strlen(response));
}

esp_err_t WifiConfigApi::ApiPingHandler(httpd_req_t *req) {
    char response[128];
    snprintf(response, sizeof(response), 
        "{\"success\":true,\"message\":\"pong\",\"timestamp\":%lld}",
        esp_timer_get_time() / 1000
    );
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    return httpd_resp_send(req, response, strlen(response));
}
