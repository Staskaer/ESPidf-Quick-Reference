#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

static const char *TAG = "scan";

static void wifi_scan(void)
{
    /*先初始化"网卡"*/
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    /* 创建存储扫描WiFi结果的列表，每个元素是wifi_ap_record_t*/
    uint16_t number = 16;
    wifi_ap_record_t ap_info[16];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    /*作为sta启动*/
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    // 开始扫描
    /*
    第一个参数数扫描配置wifi_scan_config_t，可以配置扫描
    比如只扫描选定的信道等等，没有要求则位NULL
    第二个参数是是否阻塞
    */
    esp_wifi_scan_start(NULL, true);

    /*
    获取扫描结果并存放于之前创建的列表中
    */
    esp_wifi_scan_get_ap_records(&number, ap_info);
    /*
    获取扫描到了多少个ap
    */
    esp_wifi_scan_get_ap_num(&ap_count);
    ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);

    // 遍历列表打印出所有的WiFi
    for (int i = 0; i < number; i++)
    {
        ESP_LOGW(TAG, "SSID \t\t%s", ap_info[i].ssid);
        ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
        ESP_LOGI(TAG, "Channel \t\t%d", ap_info[i].primary);
    }
}

void app_main(void)
{
    nvs_flash_init();

    wifi_scan();
}
