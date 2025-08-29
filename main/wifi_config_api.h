#ifndef WIFI_CONFIG_API_H
#define WIFI_CONFIG_API_H

#include <esp_http_server.h>

class WifiConfigApi {
public:
    static WifiConfigApi& GetInstance();
    
    // 初始化HTTP服务器
    esp_err_t Init();
    
    // 启动HTTP服务器
    esp_err_t Start();
    
    // 停止HTTP服务器
    void Stop();
    
    // 获取服务器端口
    uint16_t GetPort() const { return port_; }

private:
    WifiConfigApi() = default;
    ~WifiConfigApi() = default;
    
    // HTTP服务器句柄
    httpd_handle_t server_ = nullptr;
    uint16_t port_ = 8080;
    
    // API路由处理函数
    static esp_err_t ApiRootHandler(httpd_req_t *req);
    static esp_err_t ApiWifiScanHandler(httpd_req_t *req);
    static esp_err_t ApiWifiConnectHandler(httpd_req_t *req);
    static esp_err_t ApiWifiStatusHandler(httpd_req_t *req);
    static esp_err_t ApiWifiResetHandler(httpd_req_t *req);
    static esp_err_t ApiDeviceInfoHandler(httpd_req_t *req);
    static esp_err_t ApiDeviceStatusHandler(httpd_req_t *req);
    static esp_err_t ApiDeviceRestartHandler(httpd_req_t *req);
    static esp_err_t ApiConfigHandler(httpd_req_t *req);
    static esp_err_t ApiPingHandler(httpd_req_t *req);
    
    // 注册API路由
    esp_err_t RegisterApiRoutes();
};

#endif // WIFI_CONFIG_API_H
