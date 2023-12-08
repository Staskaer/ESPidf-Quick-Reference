#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "freertos/semphr.h"

static const char *TAG = "example";

SemaphoreHandle_t semphrHandle;

// 1s获取一个配额
void Task1(void *pvParam)
{
    int emptySpace = 0;
    BaseType_t iResult;
    while (1)
    {
        // 获取当前信号量值
        emptySpace = uxSemaphoreGetCount(semphrHandle);
        ESP_LOGI(TAG, "剩余%d\n", emptySpace);

        // 取得当前信号量,如果信号量为0则获取失败
        iResult = xSemaphoreTake(semphrHandle, 0);

        if (iResult == pdPASS)
            ESP_LOGI(TAG, "已分配\n");
        else
            ESP_LOGI(TAG, "无法分配\n");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// 6s获取一个配额
void Task2(void *pvParam)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(6000));
        xSemaphoreGive(semphrHandle);
        ESP_LOGI(TAG, "已增加配额");
    }
}

void app_main(void)
{

    semphrHandle = xSemaphoreCreateCounting(5, 5); // 创建计数型信号量，第一个参数是maxcount，第二个是initialcount

    xTaskCreate(Task1, "Task1", 1024 * 5, NULL, 1, NULL);
    xTaskCreate(Task2, "Task2", 1024 * 5, NULL, 1, NULL);
}
