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
#include "mqtt_client.h"

/*这里配置wifi的ssid与密码*/
#define wifi_ssid "test_wifi"
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

/*mqtt事件回调函数*/
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    // 获取到event
    esp_mqtt_event_handle_t event = event_data;
    // 这里可以获取到mqtt_client的句柄
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        // mqtt连接到服务端
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        // 断开连接
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        // 订阅成功
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        // 取消订阅成功
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        // 发布成功
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        // 订阅的话题有了数据
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        // 错误
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

/*初始化mqtt，并返回一个mqtt的handler*/
static esp_mqtt_client_handle_t mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        // mqtt的服务器地址和端口
        .broker.address.uri = "mqtt://192.168.43.65",
        .broker.address.port = 1883,

        // 这个是mqtt的用户名
        .credentials.username = "admin",

        // 账号和密码，在mqtt服务端可以设置校验
        .credentials.client_id = "public",
        .credentials.authentication.password = "12345678910"};
    // 初始化mqtt客户端
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    // 注册任意的mqtt事件都使用同一个回调函数处理
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    // 启动mqtt
    esp_mqtt_client_start(client);

    return client;
}

void app_main(void)
{
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();

    /*先连接wifi*/
    wifi_init_sta();
    vTaskDelay(3000 / portTICK_PERIOD_MS);

    /*启动mqtt-client*/
    esp_mqtt_client_handle_t client = mqtt_app_start();

    // 首先订阅某个topic
    /*
    第二个参数是话题的名字
    第三个参数是对应的qos
    */
    esp_mqtt_client_subscribe(client, "topic1", 1);
    esp_mqtt_client_subscribe(client, "topic2", 1);

    // 然后不断发布两个话题的订阅
    for (int i = 0;; i++)
    {
        /*
        前三个参数是对应的handler、话题名字、数据
        后三个参数是长度(0时自动计算)、qos、retain(是否需要在服务器中保留这条数据)
        */
        esp_mqtt_client_publish(client, "topic1", "topic1_data", 0, 0, 0);
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        esp_mqtt_client_publish(client, "topic2", "topic2_data", 0, 0, 0);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
