#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_event_base.h"

static const char *TAG = "example";

// 声明一个事件基，这个可以在.h文件中定义
ESP_EVENT_DECLARE_BASE(TASK_EVENTS);
// 初始化事件基
ESP_EVENT_DEFINE_BASE(TASK_EVENTS);

// 定义这个事件基中的不同事件
enum
{
    TASK_ITERATION_EVENT // raised during an iteration of the loop within the task
};

// 创建事件循环变量
esp_event_loop_handle_t loop;

/*
事件循环的回调函数
*/
static void task_iteration_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    // 提取出事件传入的参数
    int iteration = *((int *)event_data);
    ESP_LOGI(TAG, "handling %s:%s from  iteration %d", base, "TASK_ITERATION_EVENT", iteration);
}

void app_main(void)
{

    // 创建一个事件循环的参数
    esp_event_loop_args_t loop_args = {
        // 事件循环队列长度为5
        .queue_size = 5,
        // 事件循环的名字
        .task_name = "loop_task",
        // 事件循环的优先级
        .task_priority = uxTaskPriorityGet(NULL),
        // 事件循环的栈大小
        .task_stack_size = 3072,
        // 事件循环绑定到哪个cpu核心上
        .task_core_id = tskNO_AFFINITY};

    // 创建事件循环
    esp_event_loop_create(&loop_args, &loop);

    // 注册事件循环的回调函数
    // 第一个参数是绑定到我们创建的事件循环上
    // 第二个参数是事件基
    // 第三个参数是事件id
    // 第四个参数是事件循环的回调函数
    // 第五个参数是回调函数的参数
    // 最后一个参数直接为NULL即可
    esp_event_handler_instance_register_with(loop,
                                             TASK_EVENTS,
                                             TASK_ITERATION_EVENT,
                                             task_iteration_handler,
                                             loop,
                                             NULL);

    for (int iteration = 1; iteration <= 10; iteration++)
    {
        // 发送数据
        esp_event_post_to(loop, TASK_EVENTS, TASK_ITERATION_EVENT, &iteration, sizeof(iteration), portMAX_DELAY);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
