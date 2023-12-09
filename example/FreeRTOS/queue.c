#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "example";

QueueHandle_t queue;

// 1s生产一个数据
void Task1(void *pvParam)
{
    int data = 0;
    while (1)
    {
        /*两种写入方法*/
        // 一直阻塞，直到队列有空间才写入
        xQueueSend(queue, &data, portMAX_DELAY);
        // 直接写入，如果队列满了，就覆盖写入，等效于上面那个wait-time=0时
        // xQueueOverwrite(queue, &data);
        data++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// 2s消费一个数据
void Task2(void *pvParam)
{
    int data = 0;
    while (1)
    {
        /*两种读取方法*/
        // 读取后删掉已经读取的数据
        xQueueReceive(queue, &data, portMAX_DELAY);
        // 读取后不删掉已经读取的数据
        // xQueuePeek(queue, &data, portMAX_DELAY);
        ESP_LOGI(TAG, "data: %d", data);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void)
{

    queue = xQueueCreate(1, sizeof(int));

    xTaskCreate(Task1, "Task1", 1024 * 5, NULL, 1, NULL);
    xTaskCreate(Task2, "Task2", 1024 * 5, NULL, 1, NULL);
}
