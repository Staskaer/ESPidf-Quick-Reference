#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static void GPIO_init(void)
{
    // GPIO 3
    gpio_reset_pin(GPIO_NUM_3);
    // 设置输入输出模式
    gpio_set_direction(GPIO_NUM_3, GPIO_MODE_INPUT_OUTPUT);
    /*
    模式如下
    GPIO_MODE_DISABLE           --- 禁用
    GPIO_MODE_INPUT             --- 输入
    GPIO_MODE_OUTPUT            --- 输出
    GPIO_MODE_OUTPUT_OD         --- 输出开漏
    GPIO_MODE_INPUT_OUTPUT_OD   --- 输入输出开漏
    GPIO_MODE_INPUT_OUTPUT      --- 输入输出
    */

    // 设置为拉高
    gpio_set_pull_mode(GPIO_NUM_3, GPIO_PULLUP_ONLY);
    /*
    模式如下
    GPIO_PULLUP_ONLY    --- 上拉
    GPIO_PULLDOWN_ONLY  --- 下拉
    GPIO_PULLUP_PULLDOWN --- 上下拉
    GPIO_FLOATING       --- 悬空
    */

    // 中断关闭
    gpio_set_intr_type(GPIO_NUM_3, GPIO_INTR_DISABLE);
}

void app_main(void)
{
    GPIO_init();
    int level;
    while (1)
    {

        // 设置电平
        gpio_set_level(GPIO_NUM_3, 1);
        // 读取电平
        level = gpio_get_level(GPIO_NUM_3);
        ESP_LOGI("example", "GPIO[%d] level: %d\n", GPIO_NUM_3, level);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        gpio_set_level(GPIO_NUM_3, 0);
        level = gpio_get_level(GPIO_NUM_3);
        ESP_LOGI("example", "GPIO[%d] level: %d\n", GPIO_NUM_3, level);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}