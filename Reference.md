# 常用操作

## 串口日志输出

```c
#include "esp_log.h"

static const char *TAG = "example";

ESP_LOGV(TAG, "Verbose %d", 0);
ESP_LOGD(TAG, "Debug %d", 1);
ESP_LOGI(TAG, "Info %d", 2);
ESP_LOGW(TAG, "Warn %d", 3);
ESP_LOGE(TAG, "Error %d", 4);
```

## 延时

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

// 延时500ms
vTaskDelay(500 / portTICK_PERIOD_MS);
```

# 基本外设

## GPIO 

可以参考[GPIO例子](./example/basic/GPIO.c)

### 初始化

方法一：结构体设置，多个GPIO设置成同一模式

```c
#include "driver/gpio.h"

static void GPIO_init(void)
{
    gpio_config_t ioConfig = 
    {
        // 开启3、4号引脚
        .pin_bit_mask = (1ull << 3) | (1ull << 4),
        // 设置模式
        .mode = GPIO_MODE_OUTPUT,
        /*
        模式如下
        GPIO_MODE_DISABLE           --- 禁用
        GPIO_MODE_INPUT             --- 输入
        GPIO_MODE_OUTPUT            --- 输出
        GPIO_MODE_OUTPUT_OD         --- 输出开漏
        GPIO_MODE_INPUT_OUTPUT_OD   --- 输入输出开漏
        GPIO_MODE_INPUT_OUTPUT      --- 输入输出
        */

        // 拉高拉低
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        /*
        仅有xxx_DISABLE和xxx_ENABLE两种
        */

        // 中断，后面提到
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&ioConfig);
}
```

方法二：单独按需配置GPIO

```c
#include "driver/gpio.h"

static void GPIO_init(void)
{
    // GPIO 3
    gpio_reset_pin(GPIO_NUM_3);
    // 设置模式
    gpio_set_direction(GPIO_NUM_3, GPIO_MODE_INPUT);
    /*
    模式如下
    GPIO_MODE_DISABLE           --- 禁用
    GPIO_MODE_INPUT             --- 输入
    GPIO_MODE_OUTPUT            --- 输出
    GPIO_MODE_OUTPUT_OD         --- 输出开漏
    GPIO_MODE_INPUT_OUTPUT_OD   --- 输入输出开漏
    GPIO_MODE_INPUT_OUTPUT      --- 输入输出
    */

    // 拉高拉低
    gpio_set_pull_mode(GPIO_NUM_3, GPIO_PULLUP_ONLY);
    /*
    模式如下
    GPIO_PULLUP_ONLY    --- 上拉
    GPIO_PULLDOWN_ONLY  --- 下拉
    GPIO_PULLUP_PULLDOWN --- 上下拉
    GPIO_FLOATING       --- 悬空
    */

    // 中断，后面提到
    gpio_set_intr_type(GPIO_NUM_3, GPIO_INTR_DISABLE);
}
```

### 设置电平


```c
gpio_set_level(GPIO_NUM_3, 1);
gpio_set_level(GPIO_NUM_3, 0);
```

### 读取电平

```c
int value;
value = gpio_get_level(GPIO_NUM_3);
```

## GPIO中断

### 开启中断

同样有两种方法

方法一：通过结构体一次性设置

```c
#include "driver/gpio.h"

gpio_config_t ioConfig = {
        // 开启3、4号引脚
        .pin_bit_mask = (1ull << 3) ,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,

        // 设置中断
        .intr_type = GPIO_INTR_POSEDGE
        /*
        中断类型设置
        GPIO_INTR_DISABLE   --- 禁用
        GPIO_INTR_POSEDGE   --- 上升沿触发
        GPIO_INTR_NEGEDGE   --- 下降沿触发
        GPIO_INTR_ANYEDGE   --- 上升沿和下降沿触发
        GPIO_INTR_LOW_LEVEL --- 低电平触发
        GPIO_INTR_HIGH_LEVEL--- 高电平触发
        */
    };
    gpio_config(&ioConfig);
```

方法二：单独设置

```c
#include "driver/gpio.h"



static void GPIO_init(void)
{
    // GPIO 3
    gpio_reset_pin(GPIO_NUM_3);
    gpio_set_direction(GPIO_NUM_3, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_3, GPIO_PULLUP_ONLY);

    // 中断
    gpio_set_intr_type(GPIO_NUM_3, GPIO_INTR_POSEDGE);
    /*参数含义同上*/
}
```

### 中断回调函数与中断处理

需要使用`gpio_install_isr_service`函数来初始化中断服务，然后使用`gpio_isr_handler_add`函数来注册中断服务函数。在中断服务函数里不能用`ESP_LOGI`等输出日志，否则会直接coredump。
  
参见[GPIO中断例子](./example/basic/GPIO_interrupt.c)

## 定时器

### 硬件通用定时器

硬件定时器资源很少，比如esp32c3的通用定时器只有2个

参见[硬件定时器例子](./example/basic/TIM_hardware.c)

### ESP定时器

可以参考[ESP定时器例子](./example/basic/TIM_esp.c)

```c
// 回调函数
static void periodic_timer_callback(void *arg)
{
    int64_t time_since_boot = esp_timer_get_time();
    ESP_LOGI(TAG, "Periodic timer called  , time since boot: %lld us", time_since_boot);
}

void esp_init()
{
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
}

```

### FreeRTOS定时器

```c
#include "freertos/timers.h"

// 回调函数
void vTimerCallback(TimerHandle_t pxTimer)
{
    static BaseType_t xCount = 0;
    xCount++;
    ESP_LOGI(TAG, "Timer callback!   Count:%d", xCount);
}

// 创建定时器、启动
void TIM_init(void)
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

```

## 