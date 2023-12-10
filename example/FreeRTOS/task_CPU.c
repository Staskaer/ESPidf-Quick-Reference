#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <esp_log.h>

static const char *TAG = "example";

void Task1(void *param)
{
    int temp = *(int *)param; // 将传入函数的参数赋值给temp
    while (1)
    {
        ESP_LOGI(TAG, "执行任务%d", temp);
        vTaskDelay(1000 / portTICK_PERIOD_MS); // 延时1000ms
    }
};

void app_main(void)
{

    // 任务句柄，如果不需要后续操作可以不需要定义
    TaskHandle_t xHandle1 = NULL;

    int num = 1;

    xTaskCreatePinnedToCore(
        Task1,         // 定义的任务函数
        "Task1",       // 任务名称
        2048,          // 任务堆栈，使用ESP_LOG的话，太小了直接core dump
        (void *)&num,  // 输入参数
        0,             // 优先级 0-31，0为空闲任务
        &xHandle1,     // 绑定任务句柄 如果不删除或不检测任务自身信息 也可以不填入句柄直接为 NULL
        tskNO_AFFINITY // 绑定到指定CPU，0或1或tskNO_AFFINITY
        /*
        最后这里如果值为tskNO_AFFINITY，则创建的任务不会固定到任何CPU，调度程序可以在任何可用的核心上运行
        */
    );

    // 挂起任务
    vTaskDelay(2000 / portTICK_PERIOD_MS); // 2s后将Task挂起
    ESP_LOGW(TAG, "挂起任务");
    vTaskSuspend(xHandle1);

    // 恢复任务
    vTaskDelay(2000 / portTICK_PERIOD_MS); // 2S后恢复任务
    ESP_LOGW(TAG, "恢复任务");
    vTaskResume(xHandle1);

    // 删除任务
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    ESP_LOGW(TAG, "删除任务");
    vTaskDelete(xHandle1);
}
