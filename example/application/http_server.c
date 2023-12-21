#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "sdkconfig.h"
#include "esp_http_server.h"

/*这里配置wifi的ssid与密码*/
#define wifi_ssid "ppxxxg22"
#define wifi_passwd "12345678910"

static const char *TAG = "example";

/*
这里两个函数是连接wifi的
*/
void sta_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    // wifi事件组中连接wifi和连接wifi失败两个事件
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        // 连接wifi
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGW(TAG, "connected failed! retrying...");
        esp_wifi_connect();
    }

    // ip事件组中获取到ip
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI("TEST_ESP32", "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void wifi_init_sta(void)
{
    /*初始化"网卡"，前面已经完成了所以这里不需要*/
    // esp_netif_init();
    // esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // 为WIFI事件组中所有事件注册回调函数
    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &sta_event_handler,
                                        NULL,
                                        NULL);
    // 为IP事件组中获取IP注册回调函数，注意这两个是不同的事件组
    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &sta_event_handler,
                                        NULL,
                                        NULL);

    // 配置sta连接的ap的ssid和passwd，并启动wifi
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = wifi_ssid,
            .password = wifi_passwd,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "wifi_init_sta finished.");
}

/* http-server的handler
此处定义一个用于处理get请求的handler，当收到get请求时，会调用此函数
 */
esp_err_t get_handler(httpd_req_t *req)
{
    /* 发送简单的响应数据包 */
    const char resp[] = "Get Response";
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

/* http-server的handler
此处定义一个用于处理post请求的handler，当收到post请求时，会调用此函数
 */
esp_err_t post_handler(httpd_req_t *req)
{
    /* 定义 HTTP POST 请求数据的目标缓存区
     * httpd_req_recv() 只接收 char* 数据，但也可以是任意二进制数据（需要类型转换）
     * 对于字符串数据，null 终止符会被省略，content_len 会给出字符串的长度 */
    char content[100];

    /* 如果内容长度大于缓冲区则截断，然后接受 */
    size_t recv_size = MIN(req->content_len, sizeof(content));
    int ret = httpd_req_recv(req, content, recv_size);

    if (ret <= 0) /* 返回 0 表示连接已关闭 */
    {
        /* 检查是否超时 */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            /* 如果是超时，可以调用 httpd_req_recv() 重试
             * 简单起见，这里我们直接响应 HTTP 408（请求超时）错误给客户端 */
            httpd_resp_send_408(req);
        }
        /* 如果发生了错误，返回 ESP_FAIL 可以确保底层套接字被关闭 */
        return ESP_FAIL;
    }

    /* 发送简单的响应数据包 */
    const char resp[] = "POST Response";
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

/* 启动 Web 服务器的函数 */
void http_server_init(void)
{
    /*定义两个结构体，用来把对应路径、请求方法和回调函数绑定起来*/

    /*
    get请求的回调函数，当使用get方法访问/目录时，会调用get_handler
    */
    httpd_uri_t uri_get = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = get_handler,
        .user_ctx = NULL};

    /*
    post请求的回调函数，当使用post方法访问/目录时，会调用post_handler
    */
    httpd_uri_t uri_post = {
        .uri = "/",
        .method = HTTP_POST,
        .handler = post_handler,
        .user_ctx = NULL};

    // 生成http的默认配置
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* 创建一个server的handler */
    httpd_handle_t server = NULL;

    /* 启动 httpd server */
    ESP_LOGI(TAG, "starting server!");
    if (httpd_start(&server, &config) == ESP_OK)
    {
        /* 注册 URI 处理程序 */
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
    }
    /* 如果服务器启动失败，就停止server */
    if (!server)
    {
        ESP_LOGE(TAG, "Error starting server!");
        httpd_stop(server);
    }
}

void app_main(void)
{
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();

    /*先连接wifi*/
    wifi_init_sta();
    vTaskDelay(3000 / portTICK_PERIOD_MS);

    /*然后开启http服务*/
    http_server_init();
}
