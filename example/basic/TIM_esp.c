#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"

static const char *TAG = "example";
// 回调函数
static void periodic_timer_callback(void *arg)
{
    int64_t time_since_boot = esp_timer_get_time();
    ESP_LOGI(TAG, "Periodic timer called  , time since boot: %lld us", time_since_boot);
}

void app_main(void)
{
    int count = 0;

    // 创建esp定时器对象
    const esp_timer_create_args_t periodic_timer_args = {
        /*设置回调函数*/
        .callback = &periodic_timer_callback,
        /*设置回调函数的参数*/
        .arg = (void *)count,
        .name = "periodic"};
    esp_timer_handle_t periodic_timer;
    esp_timer_create(&periodic_timer_args, &periodic_timer);

    /*单次或循环的两个API*/
    // esp_timer_start_once(periodic_timer, 5000000); // 5s
    esp_timer_start_periodic(periodic_timer, 500000); // 500ms

    // 打印esp定时器信息
    esp_timer_dump(stdout);

    // 获取定时器的时间
    int64_t t1 = esp_timer_get_time();
    ESP_LOGI(TAG, "Entering light sleep for 0.5s, time since boot: %lld us", t1);

    /*停止与释放的API*/
    // ESP_ERROR_CHECK(esp_timer_stop(periodic_timer));
    // ESP_ERROR_CHECK(esp_timer_delete(periodic_timer));
}
