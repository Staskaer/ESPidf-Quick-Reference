#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"

// 定义一个队列句柄
static QueueHandle_t gpioEventQueue = NULL;

/*中断服务函数，主要加一个IRAM_ATTR*/
// 一般是通过freertos的队列发送给另一个用于处理的task来传递数据
static void IRAM_ATTR intrHandler(void *arg)
{
    // 多个GPIO中断共用一个中断服务函数时，可以通过arg来区分是哪个GPIO中断
    uint32_t gpio_num = (uint32_t)arg;
    // 发送数据到队列
    xQueueSendFromISR(gpioEventQueue, &gpio_num, NULL);

    /*
    ！！！这样写会直接coredump
    ESP_LOGI("example", "GPIO[%ld] intrrupted, level: %d\n", gpio_num, gpio_get_level(gpio_num));
    */
}

/*中断数据处理的任务*/
static void gpioTask(void *arg)
{
    uint32_t data;
    while (1)
    {
        // 队列阻塞直到收到数据并处理
        if (xQueueReceive(gpioEventQueue, &data, portMAX_DELAY))
        {
            ESP_LOGI("example", "GPIO[%ld] intrrupted, level: %d\n", data, gpio_get_level(data));
        }
    }
}

/*调用此函数来初始化*/
void app_main(void)
{
    // 初始化中断
    gpio_config_t gpio =
        {
            .pin_bit_mask = 1ull << GPIO_NUM_3,
            .mode = GPIO_MODE_INPUT,
            .intr_type = GPIO_INTR_ANYEDGE,
            /*这个设置为开启*/
            .pull_down_en = GPIO_PULLDOWN_ENABLE,
        };
    gpio_config(&gpio);

    // 创建队列
    gpioEventQueue = xQueueCreate(10, sizeof(uint32_t));

    // 创建新的task来处理中断数据
    xTaskCreate(gpioTask, "ExampleTask", 2048, NULL, 10, NULL);

    // 设置gpio中断优先级
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    // 配置GPIO中断服务 
    gpio_isr_handler_add(GPIO_NUM_3, intrHandler, (void *)GPIO_NUM_3);
}