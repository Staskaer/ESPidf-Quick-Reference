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

/*这里配置wifi的ssid与密码*/
#define wifi_ssid "test_wifi"
#define wifi_passwd "12345678910"

/*配置访问的域名、端口和目录*/
#define WEB_SERVER "www.baidu.com"
#define WEB_PORT "80"
#define WEB_PATH "/"

static const char *TAG = "example";

// 拼接出http的报文头
static const char *REQUEST = "GET " WEB_PATH " HTTP/1.0\r\n"
                             "Host: " WEB_SERVER ":" WEB_PORT "\r\n"
                             "User-Agent: esp-idf/1.0 esp32\r\n"
                             "\r\n";

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

/*获取http请求*/
static void http_get(void)
{
    /*这里有大量的错误检查相关的东西，
    因为网络连接很容易出错，不像硬件那样出错的概率相对较低
    所以这里内容看起来可能比较长*/

    // 存储域名的ip的资源
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];

    // 通过DNS获取域名的ip
    int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

    if (err != 0 || res == NULL)
    {
        ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        return;
    }
    addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

    // 创建一个socket用来建立与服务器之间的连接
    s = socket(res->ai_family, res->ai_socktype, 0);
    if (s < 0)
    {
        ESP_LOGE(TAG, "... Failed to allocate socket.");
        freeaddrinfo(res);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        return;
    }
    ESP_LOGI(TAG, "... allocated socket");

    // 因为是TCP，所以需要先进行建立连接
    if (connect(s, res->ai_addr, res->ai_addrlen) != 0)
    {
        ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
        close(s);
        freeaddrinfo(res);
        vTaskDelay(4000 / portTICK_PERIOD_MS);
        return;
    }

    ESP_LOGI(TAG, "... connected");
    freeaddrinfo(res);

    // 然后开始访问服务器，并发送GET请求
    if (write(s, REQUEST, strlen(REQUEST)) < 0)
    {
        ESP_LOGE(TAG, "... socket send failed");
        close(s);
        vTaskDelay(4000 / portTICK_PERIOD_MS);
        return;
    }
    ESP_LOGI(TAG, "... socket send success");

    // 在接受时，通过LWIP设置超时时间
    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 5;
    receiving_timeout.tv_usec = 0;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                   sizeof(receiving_timeout)) < 0)
    {
        ESP_LOGE(TAG, "... failed to set socket receiving timeout");
        close(s);
        vTaskDelay(4000 / portTICK_PERIOD_MS);
        return;
    }
    ESP_LOGI(TAG, "... set socket receiving timeout success");

    // 如果在超时时间内接收到了数据
    int index = 0;
    do
    {
        // 重置缓冲区
        bzero(recv_buf, sizeof(recv_buf));
        // 从接受区中读取数据到缓冲区然后显示出来，只打印前若干个缓冲区数据
        r = read(s, recv_buf, sizeof(recv_buf) - 1);
        for (int i = 0; i < r; i++)
        {
            putchar(recv_buf[i]);
        }
        // 打印前几个缓冲区大小的数据
        index++;
        if (index == 100)
            break;
    } while (r > 0);

    ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
    close(s);
}

void app_main(void)
{
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();

    /*先连接wifi*/
    wifi_init_sta();
    vTaskDelay(3000 / portTICK_PERIOD_MS);

    /*然后获取http数据*/
    http_get();
}
