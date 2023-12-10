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

# FreeRTOS部分

由于ESP-IDF是基于FreeRTOS的，所以FreeRTOS的API也可以直接使用，所以这里也会把FreeRTOS的常用API列出来。

## FreeRTOS任务

这里是与freeRtos相关的一些基本的常用操作，与esp-idf关系不会特别大。

### 不指定CPU创建任务(FreeRtos)

可以参考[FreeRTOS任务例子](./example/FreeRTOS/task_NOCPU.c)

```c
// 创建任务
xTaskCreate(
        Task1,        // 定义的任务函数
        "Task1",      // 任务名称
        2048,         // 任务堆栈，使用ESP_LOG的话，太小了直接core dump
        (void *)&num, // 输入参数
        2,            // 优先级 0-31，0为空闲任务
        &xHandle1     // 绑定任务句柄 如果不删除或不检测任务自身信息 也可以不填入句柄直接为 NULL
    );

// 挂起任务
vTaskSuspend(xHandle1);

// 恢复任务
vTaskResume(xHandle1);

// 删除任务
vTaskDelete(xHandle1);
```

### 指定CPU创建任务


上面那个创建不指定CPU的的API，其实是在任意一个可用CPU上创建任务，等效于此处的`tskNO_AFFINITY`参数。

可以参考[FreeRTOS任务例子](./example/FreeRTOS/task_CPU.c)


```c
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
```

## FreeRTOS队列

### 任务之间的队列

参考[FreeRTOS队列例子](./example/FreeRTOS/queue.c)

```c
#include "freertos/queue.h"
QueueHandle_t queue;

// 创建队列
queue = xQueueCreate(1, sizeof(int));

/*两种写入方法*/
// 一直阻塞，直到队列有空间才写入
xQueueSend(queue, &data, portMAX_DELAY);
// 直接写入，如果队列满了，就覆盖写入，等效于上面那个wait-time=0时
xQueueOverwrite(queue, &data);

/*两种读取方法*/
// 读取后删掉已经读取的数据
xQueueReceive(queue, &data, portMAX_DELAY);
// 读取后不删掉已经读取的数据
xQueuePeek(queue, &data, portMAX_DELAY);
```

### 中断函数之间的队列

在中断中使用的API有所区别，但是参数基本不变。

```c
// 发送
xQueueSendFromISR(QueueHandler, &data, NULL);

// 接受
xQueueReceiveFromISR(QueueHandler, &data, NULL);
```

## FreeRTOS定时器

[FreeRTOS定时器例子](./example/FreeRTOS/TIM_freertos.c)

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



## 信号量

### 信号量/互斥量

可以参见[信号量例子](./example/FreeRTOS/sem.c)

```c
#include "freertos/semphr.h"
// 创建信号量两者方法等效
// semphrHandle = xSemaphoreCreateMutex();
semphrHandle = xSemaphoreCreateBinary(); // 1.创建信号量
xSemaphoreGive(semphrHandle);            // 2.释放信号量，创建完成后需要立刻释放一次

// 获取信号量
xSemaphoreTake(semphrHandle, portMAX_DELAY); 

// 释放信号量
xSemaphoreGive(semphrHandle); 
```

### 计数信号量

```c
// 创建计数型信号量，第一个参数是maxcount，第二个是initialcount
// 不需要释放一次
semphrHandle = xSemaphoreCreateCounting(5, 5);

// 获取当前信号量值
emptySpace = uxSemaphoreGetCount(semphrHandle);

// 立刻取得当前信号量并返回,如果信号量为0则获取失败
iResult = xSemaphoreTake(semphrHandle, 0);

// 释放一次信号量
xSemaphoreGive(semphrHandle);
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

参见[GPIO中断例子](./example/basic/GPIO_interrupt.c)

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
  


## 定时器

如果想要查看freeRtos中的软定时器，查阅`FreeRTOS部分`

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


## pwm

### 生成两路pwm

可以参考[pwm例子](./example/basic/pwm.c)

```c
// 先初始化pwm对应的定时器
static const ledc_timer_config_t lcd_bl_ledc_timer = 
{
    .duty_resolution = LEDC_TIMER_10_BIT, // LEDC驱动器占空比精度
    .freq_hz = 2000,                      // PWM频率
    .speed_mode = LEDC_LOW_SPEED_MODE,    // 选择PWM频率的方式，有高速和低速两种
    .timer_num = LEDC_TIMER_0,            // ledc使用的定时器编号。若需要生成多个频率不同的PWM信号，则需要指定不同的定时器
    /*
    一共有4个timer，通过总线矩阵对应到8个GPIO输出上
    一个timer可以对应多个channel，但是一个channel只能对应一个timer
    因此，8个channel最多有4种不同频率的pwm输出
    */
    .clk_cfg = LEDC_AUTO_CLK, // 自动选择定时器的时钟源
    /*
    LEDC_AUTO_CLK：自动选择定时器的时钟源
    LEDC_USE_APB_CLK：使用APB时钟作为定时器时钟源，APB时钟为80MHz
    LEDC_USE_RTC8M_CLK：使用RTC8M时钟作为定时器时钟源，RTC8M时钟为8MHz
    LEDC_USE_REF_TICK：使用REF_TICK时钟作为定时器时钟源，REF_TICK时钟为1MHz
    */
};

/* 初始化定时器1，将初始化好的定时器编号传给ledc通道初始化函数即可 */
ledc_timer_config(&lcd_bl_ledc_timer);


// 然后初始化对应通道和GPIO
ledc_channel_config_t lcd_bl_ledc_channel0 = 
{
    .channel = LEDC_CHANNEL_0,         // LED使用通道0
    .duty = 0,                         // 占空比0
    .gpio_num = 3,                     // 连接LED的IO
    .speed_mode = LEDC_LOW_SPEED_MODE, // 速率
    .hpoint = 0,                       // 没啥用，不用管
    .timer_sel = LEDC_TIMER_0,         // 使用上面初始化过的定时器
};
ledc_channel_config(&lcd_bl_ledc_channel0);


/* 设定PWM占空比 */
ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, brightness * 10);

/* 更新PWM占空比输出 */
ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
```

### pwm控制sg90舵机

通过配置相关的pwm参数即可完成控制，参考[舵机例子](./example/basic/pwm_servo.c)

## UART

可以直接使用串口来读取发送数据，也可以通过事件回调的方式来使用串口。

### 直接使用串口

可以参考[串口例子](./example/basic/uart.c)

```c
#include "driver/uart.h"
/*安装uart的驱动
第一个参数选择使用的第几个串口
第二个参数是接收缓冲区的大小，字节为单位
第三个参数是发送缓冲区的大小，字节为单位
第四第五个参数是事件相关的，参见下一个使用事件的例子
最后一个是中断，这里不分配
*/
uart_driver_install(UART_NUM_1, 2048, 0, 0, NULL, 0);


/*初始化串口*/
uart_config_t uart_config = {
    /*波特率*/
    .baud_rate = 115200,
    /*数据位*/
    .data_bits = UART_DATA_8_BITS,
    /*校验*/
    .parity = UART_PARITY_DISABLE,
    /*停止位*/
    .stop_bits = UART_STOP_BITS_1,
    /*串口(cts/rts)相关，不管即可*/
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    /*时钟源，两种可选：UART_SCLK_APB(默认，也就是UART_SCLK_DEFAULT)、UART_SCLK_REF_TICK*/
    .source_clk = UART_SCLK_DEFAULT,
};
/*这里把使用的串口完成配置*/
uart_param_config(UART_NUM_1, &uart_config);

/*然后去配置这个串口的引脚
第一个参数是使用的串口，需要和前面两个设置保持一致
第二个参数是TXD的引脚号，如果使用默认的，就填UART_PIN_NO_CHANGE
第三个参数是RXD的引脚号，如果使用默认的，就填UART_PIN_NO_CHANGE
最后两个参数是cts/rts相关，不管即可，填UART_PIN_NO_CHANGE
*/
uart_set_pin(UART_NUM_1,
                GPIO_NUM_3,
                GPIO_NUM_4,
                UART_PIN_NO_CHANGE,
                UART_PIN_NO_CHANGE);

// 分配缓冲区
uint8_t data[1024];
// 读取串口数据
int len = uart_read_bytes(UART_NUM_1, &data, (1024 - 1), 20 / portTICK_PERIOD_MS);
// 发送串口数据
uart_write_bytes(UART_NUM_1, &data, len);
```

### 通过事件来使用串口

这个稍微有点复杂，可以参考[串口事件例子](./example/basic/uart_event.c)

这里的事件会被内置的中断处理，需要使用一个单独的任务来处理串口事件，主要是多了一个检测某条命令的末尾是不是有特定字符重复特定次数的功能。

## ADC【暂无】

## IIC【暂无】

## DAC【暂无】

## SPI【暂无】


# 无线通信

## WIFI【暂无】

## 蓝牙【暂无】

# 应用层协议

## HTTP【暂无】

## MQTT【暂无】