#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "freertos/semphr.h"

static const char *TAG = "pv";

int pv = 0;

SemaphoreHandle_t semphrHandle; // 创建一个SemaphoreHandle_t类型句柄

void Task1(void *pvParam)
{
    while (1)
    {
        xSemaphoreTake(semphrHandle, portMAX_DELAY); // 获取信号量

        // 任务1将在5s内占用信号量，5s后释放，此时任务2无法获取信号量从而任务无法执行
        for (int i = 0; i < 5; i++)
        {
            pv++;
            ESP_LOGI(TAG, "Task1 pv = %d!\n", pv);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        xSemaphoreGive(semphrHandle); // 释放信号量
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
void Task2(void *pvParam)
{
    while (1)
    {
        xSemaphoreTake(semphrHandle, portMAX_DELAY); // 获取信号量

        // 任务2将在5s内占用信号量，5s后释放，此时任务1无法获取信号量从而导致任务无法执行
        for (int i = 0; i < 5; i++)
        {
            pv++;
            ESP_LOGI(TAG, "Task2 pv = %d!\n", pv);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        xSemaphoreGive(semphrHandle); // 释放信号量
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    /*这两种方法等效*/
    // semphrHandle = xSemaphoreCreateMutex();
    semphrHandle = xSemaphoreCreateMutex(); // 1.创建信号量
    xSemaphoreGive(semphrHandle);           // 2.释放信号量，创建完成后需要立刻释放一次

    // 创建任务
    xTaskCreate(Task1, "Task1", 1024 * 5, NULL, 1, NULL);
    xTaskCreate(Task2, "Task2", 1024 * 5, NULL, 1, NULL);
}
