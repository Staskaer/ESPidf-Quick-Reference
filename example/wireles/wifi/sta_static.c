#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include <netdb.h>
#include "nvs_flash.h"

static const char *TAG = "static_ip";

static void set_dns_server(esp_netif_t *netif, uint32_t addr, esp_netif_dns_type_t type)
{
    // 创建一个dns对象并设置参数
    esp_netif_dns_info_t dns;
    dns.ip.u_addr.ip4.addr = addr;
    dns.ip.type = IPADDR_TYPE_V4;
    // 对"网卡"配置dns
    esp_netif_set_dns_info(netif, type, &dns);
}

static void set_static_ip_and_dns(esp_netif_t *netif)
{
    // 先关闭dhcp
    esp_netif_dhcpc_stop(netif);

    // 设置ip
    esp_netif_ip_info_t ip;
    memset(&ip, 0, sizeof(esp_netif_ip_info_t));
    // 目标静态ip地址、子网掩码、网关
    ip.ip.addr = ipaddr_addr("192.168.43.23");
    ip.netmask.addr = ipaddr_addr("255.255.255.0");
    ip.gw.addr = ipaddr_addr("192.168.43.1");

    // 正式设置静态ip
    esp_netif_set_ip_info(netif, &ip);

    ESP_LOGD(TAG, "Success to set static ip, netmask, gw");

    // 配置主dns和备用dns
    set_dns_server(netif, ipaddr_addr("192.168.43.1"), ESP_NETIF_DNS_MAIN);
    set_dns_server(netif, ipaddr_addr("114.114.114.114"), ESP_NETIF_DNS_BACKUP);
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{

    // wifi事件组中连接wifi和连接wifi失败两个事件，以及在sta连接成功时配置ip
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        // 连接wifi
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        // 配置静态ip和dns
        set_static_ip_and_dns(arg);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        // 重连
        ESP_LOGW(TAG, "connected failed! retrying...");
        esp_wifi_connect();
    }

    // ip事件组中获取到ip
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "static ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void wifi_init_sta(void)
{
    // 各种初始化...
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // 为ip和wifi事件注册回调函数，需要把"sta网卡"传递进去，也就是sta_netif
    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &event_handler,
                                        sta_netif,
                                        NULL);
    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &event_handler,
                                        sta_netif,
                                        NULL);

    // 配置wifi参数
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "ppxxxg22",
            .password = "12345678910",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    // 启动wifi
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "wifi_init_sta finished.");
}

void app_main(void)
{

    nvs_flash_init();

    wifi_init_sta();
}