#include "sdkconfig.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_wifi.h"

/*这里配置wifi的ssid与密码*/
#define wifi_ssid "wifi_test"
#define wifi_passwd "12345678910"

static const char *TAG = "example";
static const char *payload = "Message from ESP32 ";

/* 这里两个函数是连接wifi的
会连接到test_wifi里，并且假设该wifi内存在一台TCP-server，地址为192.168.43.65:8899
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

/*创建并连接wifi*/
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

/*下面创建TCP socket*/
void tcp_client(void)
{
    char rx_buffer[128];
    char host_ip[] = "192.168.43.65";
    int addr_family = 0;
    int ip_protocol = 0;

    // 创建一个ipv4的地址
    struct sockaddr_in dest_addr;
    // 把字符串类型的地址赋值，并设置类型为TCP(AF_INET)
    inet_pton(AF_INET, host_ip, &dest_addr.sin_addr);
    dest_addr.sin_family = AF_INET;
    // 这里设置对应的端口号
    dest_addr.sin_port = htons(8899);

    // 然后创建对应的socket
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    int sock = socket(addr_family, SOCK_STREAM, ip_protocol);

    ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, 8899);

    // 等待wifi连接成功
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    // 在连接时容易发生错误
    if (err != 0)
    {
        ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        return;
    }
    ESP_LOGI(TAG, "Successfully connected");

    /*连接成功后下面开始发送数据*/

    while (1)
    {
        /*发送数据*/
        int err = send(sock, payload, strlen(payload), 0);
        if (err < 0)
        {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            break;
        }

        /*接受数据到缓存区*/
        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0)
        {
            ESP_LOGE(TAG, "recv failed: errno %d", errno);
            break;
        }

        /*接受到数据后打印*/
        else
        {
            rx_buffer[len] = 0;
            ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
            ESP_LOGI(TAG, "%s", rx_buffer);
        }
    }

    /*如果sock出错，需要先停止、关闭然后才能重复创建*/
    // shutdown(sock, 0);
    // close(sock);
}

void app_main(void)
{
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();

    /*先连接wifi*/
    wifi_init_sta();
    /*再尝试TCP通信*/
    tcp_client();
}