#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

static const char *TAG = "wifi softAP";

/*wifi事件的回调函数，目前只处理有设备连接到当前ap或者从当前ap断开时
处理的事件需要事先在事件循环中注册
*/
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    // 有设备连接到当前ap，打印设备的mac地址和ip地址
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    // 有设备从当前ap断开，打印设备的mac地址
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void wifi_init_softap(void)
{
    // 初始化ip/tcp协议栈
    esp_netif_init();

    // 创建默认的事件循环
    esp_event_loop_create_default();

    // 创建好wifi-ap的网络接口
    esp_netif_create_default_wifi_ap();

    // 使用默认的wifi初始化配置
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // 注册wifi事件的回调函数，任意事件都会被注册，但是回调函数只处理两类事件
    esp_event_handler_instance_register(WIFI_EVENT,
                                        // 也可以是指定事件，比如WIFI_EVENT_AP_STACONNECTED
                                        ESP_EVENT_ANY_ID,
                                        /*对应的回调函数*/
                                        &wifi_event_handler,
                                        NULL,
                                        NULL);

    // 配置ap相关的东西
    wifi_config_t wifi_config = {
        .ap = {
            // ap的ssid
            .ssid = "ESP32",
            // ap的ssid长度
            .ssid_len = strlen("ESP32"),
            // ap使用的信道，1-13
            .channel = 1,
            // ap密码
            .password = "12345678",
            // ap的最大连接数目
            .max_connection = 4,
            /*ap的加密方式，一般两个常用
            WIFI_AUTH_OPEN：不加密
            WIFI_AUTH_WPA2_PSK: WPA2-PSK加密
             */
            .authmode = WIFI_AUTH_WPA2_PSK,
            // ap的ssid是否隐藏
            .ssid_hidden = 0,
            // 受保护的管理帧
            .pmf_cfg = {
                .required = true,
            },
        },
    };

    // 设置wifi模式为ap模式
    esp_wifi_set_mode(WIFI_MODE_AP);
    // 并把wifi按照上面的描述进行配置
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);

    // wifi，启动
    esp_wifi_start();

    ESP_LOGI(TAG, "wifi_init_softap finished.");
}

void app_main(void)
{
    // 初始化flash的nvs分区，因为wifi相关配置会用到这个分区
    nvs_flash_init();

    wifi_init_softap();
}
