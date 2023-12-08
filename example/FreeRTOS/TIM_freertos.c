#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"

static const char *TAG = "example";

void vTimerCallback(TimerHandle_t pxTimer)
{
    static BaseType_t xCount = 0;
    xCount++;
    ESP_LOGI(TAG, "Timer callback!   Count:%d", xCount);
}

void app_main(void)
{
    // 如果成功创建了计时器，则返回新创建的计时器的句柄
    TimerHandle_t tim = xTimerCreate(
        "vTimerCallback", // 名称
        100,              // 周期时长 系统滴答数
        pdTRUE,           // 是否重新加载，pdFALSE为单次调用
        0,                // pvTimerID
        vTimerCallback    // 定义的定时器回调函数
    );
    xTimerStart(tim, 0); // 激活成功返回 pdPASS
    // 第1个参数：软件计时器的句柄
    // 第2个参数：指定在调用xTimerStart（）时，如果队列已满，
    // 则调用任务应保持在“阻塞”状态
    // 以等待开始命令成功发送到计时器命令队列的时间（以秒为单位）。
}
