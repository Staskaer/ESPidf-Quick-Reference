#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gptimer.h"
#include "esp_log.h"

static const char *TAG = "example";

// 定时器回调函数队列
QueueHandle_t TIM_queue = NULL;
// 定时器句柄
gptimer_handle_t gptimer = NULL;

// 定时器回调函数
static bool IRAM_ATTR TIM_Callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;
    QueueHandle_t queue = (QueueHandle_t)user_data;

    /******这里获取到定时器当前时钟并传递给队列********/
    int ele = edata->count_value;
    xQueueSendFromISR(queue, &ele, &high_task_awoken);
    /***********************************************/

    return (high_task_awoken == pdTRUE);
}

static void TIM_init()
{
    // 初始化定时器队列
    TIM_queue = xQueueCreate(10, sizeof(int));

    gptimer_config_t timer_config = {
        /*默认就这个，不用改*/
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,

        // 计数方向
        .direction = GPTIMER_COUNT_UP,
        /*
        GPTIMER_COUNT_DOWN  -- 向下计数
        GPTIMER_COUNT_UP    -- 向上计数
        */

        /* 分频，下面这个是1us/tick */
        .resolution_hz = 1000000, // 1MHz, 1 tick=1us
    };
    // 初始化定时器
    gptimer_new_timer(&timer_config, &gptimer);

    // 下面配置回调函数
    gptimer_event_callbacks_t cbs = {
        .on_alarm = TIM_Callback,
    };
    gptimer_register_event_callbacks(gptimer, &cbs, TIM_queue);
    // 使能定时器
    gptimer_enable(gptimer);

    // 配置重载相关参数
    gptimer_alarm_config_t alarm_config = {
        /*自动重载后数值*/
        .reload_count = 0,
        /*到达该数值后报警*/
        .alarm_count = 1000000, // period = 1s
        /*是否开启自动重载*/
        .flags.auto_reload_on_alarm = true,
    };
    gptimer_set_alarm_action(gptimer, &alarm_config);
}

void app_main(void)
{
    TIM_init();
    int ele;

    // 开启定时器计数
    gptimer_start(gptimer);
    while (1)
    {
        if (xQueueReceive(TIM_queue, &ele, pdMS_TO_TICKS(2000)))
        {
            ESP_LOGI(TAG, "Timer reloaded, count=%d", ele);
        }
        else
        {
            ESP_LOGW(TAG, "Missed one count event");
        }
    }
    /* 其他一些api
    设置定时器计数值
    gptimer_set_raw_count(gptimer, 100);

    获取定时器计数值
    uint64_t count;
    gptimer_get_raw_count(gptimer, &count);

    使能、禁用定时器
    gptimer_disable(gptimer);
    gptimer_enable(gptimer);

    开启、关闭定时器的计数
    gptimer_start(gptimer);
    gptimer_stop(gptimer);

    删除定时器
    gptimer_del_timer(gptimer);
    */
}