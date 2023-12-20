#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "esp_wifi.h"

/*这里配置wifi的ssid与密码*/
#define wifi_ssid "wifi_test"
#define wifi_passwd "12345678910"

static const char *TAG = "example";
static const char *payload = "Message from ESP32 ";

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

/*这个是udp-client线程
会首先创建一个socket，然后不断发送数据，接受数据
*/
static void udp_client_task(void *pvParameters)
{
    char rx_buffer[128];

    // 设置目标地址、协议与端口
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr("192.168.43.65");
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(8899);
    // 创建出一个UDP协议的socket
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }

    // 设置超时时间，第一个为s，第二个为ms，是LWIP中的参数
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

    ESP_LOGI(TAG, "Socket created, sending to %s:%d", "192.168.43.65", 8899);

    // 不断通过UDP发送、接受数据
    while (1)
    {

        // 发送数据并检查是否发送成功
        int err = sendto(sock, payload, strlen(payload), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0)
        {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Message sent");

        // 接受数据
        struct sockaddr_storage source_addr;
        socklen_t socklen = sizeof(source_addr);
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

        // 没有数据，可能是超时等因素
        if (len < 0)
        {
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
            continue;
        }
        // 接受数据到缓冲区，并显示
        else
        {
            rx_buffer[len] = 0;
            ESP_LOGI(TAG, "Received %d bytes from %s:", len, "192.168.43.65");
            ESP_LOGI(TAG, "%s", rx_buffer);
            if (strncmp(rx_buffer, "OK: ", 4) == 0)
            {
                ESP_LOGI(TAG, "Received expected message, reconnecting");
                continue;
            }
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    // 如果socket出错，想要重新配置socket时要先停止和关闭
    // shutdown(sock, 0);
    // close(sock);
    // 重新创建...
}

void app_main(void)
{
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();

    /*先连接wifi*/
    wifi_init_sta();
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    /*再创建UDP-client*/
    xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 5, NULL);
}