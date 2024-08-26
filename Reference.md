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


上面那个创建不指定CPU的的API，其实是在任意一个可用CPU上创建任务，等效于此处的`tskNO_AFFINITY`参数。**有浮点运算的任务务必要指定CPU**，最好wifi、蓝牙等网络IO任务一个核，剩余的任务另外一个核心。

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

硬件定时器资源很少，类似stm32的基本定时器，只能计数，比如ESP32内置4个64-bit通用定时器。每个定时器包含一个16-bit预分频器和一个64-bit可自动重新加载向上／向下计数器。

其他比如led pwm也会占用定时器资源，所以在使用时要注意。


参见[硬件定时器例子](./example/basic/TIM_hardware.c)

### ESP高分辨率定时器

比硬件通用定时器分辨率更高，而且能进行中断管理。

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

## ADC

### 无DMA的采样与校准

ADC需要通过内置的efuse进行校准，ADC的默认量程为0-1.1v，所以根据不同的需要量程来配置对应的衰减，使用时可以直接读取对应通道数值，也可以读取校准后的数值，可以参考[ADC例子](./example/basic/ADC.c)

```c
/*这里只是初始化相关代码，校准相关参考对应文件*/

// 初始化ADC1
adc_oneshot_unit_handle_t adc1_handle;
adc_oneshot_unit_init_cfg_t init_config1 = {
    // 使用哪个ADC
    .unit_id = ADC_UNIT_1,
    // 设置是否支持 ADC 在 ULP (超低功耗)模式下工作
    .ulp_mode = ADC_ULP_MODE_DISABLE};
adc_oneshot_new_unit(&init_config1, &adc1_handle);

// 配置ADC的采样设置
adc_oneshot_chan_cfg_t config = {
    /*
    位宽，也就是精度相关
    ADC_BITWIDTH_DEFAULT：默认位宽
    ADC_BITWIDTH_9：9位精度
    ...
    ADC_BITWIDTH_12：12位精度
    */
    .bitwidth = ADC_BITWIDTH_DEFAULT,
    /*配置衰减，是把输入电压缩小若干倍后送入ADC
    不同的衰减倍数对应不同的检测电压范围，默认ADC的量程0-1.1V

    ADC_ATTEN_DB_0：不衰减，测量范围为0-1.1V
    ADC_ATTEN_DB_2_5：输入衰减2.5dB(量程*1.33)，测量范围为0-1.5V
    ADC_ATTEN_DB_6：衰减6dB(量程*2)，测量范围为0-2.2V
    ADC_ATTEN_DB_11：衰减11dB(量程*3.55)，测量范围为0-3.9V

    这里配置成了ADC_ATTEN_DB_11，也就是最大量程
    */
    .atten = ADC_ATTEN_DB_11,
};

// 初始化两个通道，分别是2通道和3通道
adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_2, &config);
adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &config);
```

### 通过DMA的连续采样

首先需要配置dma的数据类型，由于不同芯片的支持的程度不一样，所以需要根据自己的芯片来配置。然后完成DMA和ADC对应通道的初始化，主要还是针对adc来进行初始化，在adc的初始化中指定对应的通道以dma来进行传输即可，可以参考[例子](./example/basic/ADC_DMA.c)

```c
/*
这里配置DMA的类型，这里最好转到adc_digi_output_data_t里查看，比较混乱

ADC_DIGI_OUTPUT_FORMAT_TYPE1：会从adc_digi_output_data_t->type1中获取数据
ADC_DIGI_OUTPUT_FORMAT_TYPE2：会从adc_digi_output_data_t->type2中获取数据

但是不同芯片的支持adc_digi_output_data_t结构是有差异的


//////////// esp32与esp32s2 ////////////
adc_digi_output_data_t数据结构体中有type1和type2，都是16位
其中type1中12位数据位+4位通道标识位
type2中11位数据位+4位通道标识位+1位adc标识位

esp32：仅支持type1，且adc2不支持dma
esp32s2：支持type1和type2，adc2支持dma，所以可以使用type2来分辨是哪个adc采集的数据

//////////// esp32c3、h2、c2 ////////////
adc_digi_output_data_t数据结构体仅有type2，是32位
type2由12位数据位+3位通道位+1位adc标识位+16位保留位组成

由于仅有一个type2，所以这些芯片仅支持ADC_DIGI_OUTPUT_FORMAT_TYPE2

//////////// esp32s3 ////////////
adc_digi_output_data_t数据结构体仅有type2，是32位
type2由12位数据位+4位通道位+1位adc标识位+15位保留位组成

由于仅有一个type2，所以这些芯片仅支持ADC_DIGI_OUTPUT_FORMAT_TYPE2


//////////// else ////////////
但是adc_digi_output_data_t是对齐的，所以只需要在初始化的时候关注就好了

这里是c3，所以使用的是type2的三个宏，分别定义了type2时的通道获取、数据获取、adc单元获取
如果是type1的话，就是type1.channel、type1.data两种数据可以获取
*/
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
#define ADC_DMA_OUTPUT_TYPE ADC_DIGI_OUTPUT_FORMAT_TYPE1
#define ADC_GET_CHANNEL(p_data) ((p_data)->type1.channel)
#define ADC_GET_DATA(p_data) ((p_data)->type1.data)
#else
#define ADC_DMA_OUTPUT_TYPE ADC_DIGI_OUTPUT_FORMAT_TYPE2
#define ADC_GET_CHANNEL(p_data) ((p_data)->type2.channel)
#define ADC_GET_DATA(p_data) ((p_data)->type2.data)
#define EXAMPLE_ADC_GET_UNIT(p_data) ((p_data)->type2.unit)
#endif

/*
配置ADC_DMA的最大存储缓冲区大小和每个转换帧大小
*/
adc_continuous_handle_cfg_t adc_config = {
    .max_store_buf_size = 1024,
    .conv_frame_size = 256,
};
adc_continuous_new_handle(&adc_config, &handle);

/* 上面配置好了ADC_DMA的模式，下面来配置每一个ADC IO的采样配置
sample_freq_hz：采样频率，这里是20KHz
conv_mode：转换模式，这里是ADC_CONV_SINGLE_UNIT_1，也就是只使用ADC1
format：DMA输出格式，这里是ADC_DIGI_OUTPUT_FORMAT_TYPE2，也就是type2
pattern_num：采样通道数，这里是2


除了这些外，还有一个adc_pattern需要为每个ADC IO配置衰减、通道号、adc单元等采样配置
adc_pattern.atten：衰减
adc_pattern.channel：对应的adc通道
adc_pattern.unit：对应的adc单元
adc_pattern.bit_width：位宽

adc_pattern需要为每个进行dma传输的adc进行配置
    */
adc_continuous_config_t dig_cfg = {
    .sample_freq_hz = 20 * 1000,
    .conv_mode = ADC_CONV_SINGLE_UNIT_1,
    .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    .pattern_num = 2,
};

// 这里来配置adc_pattern，与adc单独使用时的参数一致
adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
for (int i = 0; i < channel_num; i++)
{
    adc_pattern[i].atten = ADC_ATTEN_DB_11;
    adc_pattern[i].channel = channel[i] & 0x7;
    adc_pattern[i].unit = ADC_UNIT_1;
    adc_pattern[i].bit_width = ADC_BITWIDTH_12;

    ESP_LOGI(TAG, "adc_pattern[%d].atten is :%" PRIx8, i, adc_pattern[i].atten);
    ESP_LOGI(TAG, "adc_pattern[%d].channel is :%" PRIx8, i, adc_pattern[i].channel);
    ESP_LOGI(TAG, "adc_pattern[%d].unit is :%" PRIx8, i, adc_pattern[i].unit);
}
// 然后完成配置并进行初始化
dig_cfg.adc_pattern = adc_pattern;
adc_continuous_config(handle, &dig_cfg);

```

## DAC

**esp32c3没有DAC，所以此例程基于esp32**。DAC只需要指定对应的通道即可完成配置，然后就可以不断写入8位数据来表示输出电压；另外DAC还有一个余弦发生器可以用来生成正弦波，可以参考[例子](./example/basic/DAC.c)

```c
#include "driver/dac.h"

// 初始化对应通道
dac_output_enable(DAC_CHANNEL_2);

// 写入数据
dac_output_voltage(DAC_CHANNEL_2, 0x7f);


// 使用余弦发生器
dac_cw_config_t config = {
    // 使用DAC通道1
    .en_ch = DAC_CHANNEL_1,
    // 10KHz正弦波
    .freq = 10000,
    // DAC_CW_SCALE_1、2、4、8表示赋值分别为3.3v的1/n倍
    // 这里的赋值是不变
    .scale = DAC_CW_SCALE_1,
    // 相位为0，可选DAC_CW_PHASE_0、DAC_CW_PHASE_180
    .phase = DAC_CW_PHASE_0,
};
dac_cw_generator_config(&config);
// 打开余弦波发生器
dac_cw_generator_enable();
// 使能通道
dac_output_enable(DAC_CHANNEL_1);
```


## 脉冲计数【暂无】

## 红外遥控【暂无】

## SDIO【暂无】

## I2C【暂无】

## SPI【暂无】

## I2S【暂无】

## USB【搁置】

因为esp32C3上的USB只用于烧录固件和调试，不支持通用USB功能开发，而手上又没有esp32s2之类的板子，没法验证，所以这里先搁置了。


# 无线通信

## WIFI

### AP

建议直接参考[AP例子](./example/wireles/wifi/soft_ap.c)，整个操作较多。

### STA

建议直接参考[扫描例子](./example/wireles/wifi/sta.c)，与ap相类似，初始化相关较多。

### STA扫描

参考[扫描例子](./example/wireles/wifi/scan.c)，在配置成STA的模式后调用相关函数来扫描AP

```c
// 扫描结果存放
wifi_ap_record_t ap_info[16];

/*作为sta启动*/
esp_wifi_set_mode(WIFI_MODE_STA);
esp_wifi_start();

// 开始扫描
/*
第一个参数数扫描配置wifi_scan_config_t，可以配置扫描
比如只扫描选定的信道等等，没有要求则位NULL
第二个参数是是否阻塞
*/
esp_wifi_scan_start(NULL, true);

/*
获取扫描结果并存放于之前创建的列表中
*/
esp_wifi_scan_get_ap_records(&number, ap_info);

// 可以通过遍历方式来获取到每个ap的信道、ssid等信息
```

### STA静态IP与DNS

请直接参考参考[静态IP例子](./example/wireles/wifi/sta_static.c)，内容较长。


## TCP&UDP

### WIFI连接后的TCP-Client

一般都是先连接好WIFI，然后TCP再基于此进行相互通信，可以查看[例子](./example/wireles/socket/TCP_client.c)，当然在使用前也需要完成"网卡"、事件循环、nvs分区的初始化操作

```c
char host_ip[] = "192.168.43.65";

// 创建一个ipv4的地址
struct sockaddr_in dest_addr;
// 把字符串类型的地址赋值，并设置类型为TCP(AF_INET)
inet_pton(AF_INET, host_ip, &dest_addr.sin_addr);
dest_addr.sin_family = AF_INET;
// 这里设置对应的端口号
dest_addr.sin_port = htons(8899);

// 然后创建对应的socket
int addr_family = AF_INET;
int ip_protocol = IPPROTO_IP;
int sock = socket(addr_family, SOCK_STREAM, ip_protocol);

// 连接并检查socket
int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
// 在连接时容易发生错误
if (err != 0)
{
    ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
    return;
}
```

### WIFI连接后的TCP-Server

首先还是需要连接上WIFI，然后在esp32上创建一个监听socket，一旦有数据传输进来就再创建一个用于传输的socket用于通信，这里涉及到了一些LWIP库相关的参数，整体也相对复杂，并且在配置时很容易出错，建议直接参考[例子](./example/wireles/socket/TCP_server.c)

如果是两台ESP32通信的话，其中一台会配置成AP，只需要参照之前的AP例子，更换wifi的配置模式，然后就可以进行通信了。


### WIFI连接后的UDP-Client

同样，一般是首先配置好wifi，然后配置socket，这些步骤与TCP基本一致，只有个别一些参数有所不同，可以参考[例子](./example/wireles/socket/UDP_client.c)

### WIFI连接后的UDP-Server

同上，与UDP-Client十分类似，可以参考[例子](./example/wireles/socket/UDP_server.c)

如果是两台ESP32通信的话，其中一台会配置成AP，只需要参照之前的AP例子，更换wifi的配置模式，然后就可以进行通信了。

## ESP-Now【暂无】

## 蓝牙【搁置】

esp32C3仅支持BLE，也就是低功耗蓝牙，但是由于我用的不多，所以暂时这部分内容也先搁置了

# 应用层协议

## HTTP

### HTTP-client

发送Http的get请求，需要首先通过DNS来获取到域名的ip，然后才能建立连接并发送请求，整个代码相对较长，也需要先连接到wifi中，可以参考[例子](./example/application/http_client.c)

### HTTP-server

创建http server来处理get和post请求，需要注册对用的回调函数即可，[例子](./example/application/http_server.c)

```c
/* http-server的handler
此处定义一个用于处理get请求的handler，当收到get请求时，会调用此函数
 */
esp_err_t get_handler(httpd_req_t *req)
{
    /* 发送简单的响应数据包 */
    const char resp[] = "Get Response";
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

/* 启动 Web 服务器的函数 */
void http_server_init(void)
{
    /*
    get请求的回调函数，当使用get方法访问/目录时，会调用get_handler
    */
    httpd_uri_t uri_get = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = get_handler,
        .user_ctx = NULL};

    // 生成http的默认配置
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* 创建一个server的handler */
    httpd_handle_t server = NULL;

    /* 启动 httpd server */
    ESP_LOGI(TAG, "starting server!");
    if (httpd_start(&server, &config) == ESP_OK)
        /* 注册 URI 处理程序 */
        httpd_register_uri_handler(server, &uri_get);
    
    /* 如果服务器启动失败，就停止server */
    if (!server)
    {
        ESP_LOGE(TAG, "Error starting server!");
        httpd_stop(server);
    }
}
```


## MQTT

首先需要配置好mqtt的服务端，然后在板子中配置好wifi，然后就可以使用mqtt发送数据来完成通信，同样是基于事件来完成的，可以参考[例子](./example/application/mqtt_client.c)

```c
/*mqtt事件回调函数*/
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        // mqtt连接到服务端
        break;
    case MQTT_EVENT_DATA:
        // 订阅的话题有了数据
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    }
}

/*初始化mqtt，并返回一个mqtt的handler*/
static esp_mqtt_client_handle_t mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        // mqtt的服务器地址和端口
        .broker.address.uri = "mqtt://192.168.43.65",
        .broker.address.port = 1883,

        // 这个是mqtt的用户名
        .credentials.username = "admin",

        // 账号和密码，在mqtt服务端可以设置校验
        .credentials.client_id = "public",
        .credentials.authentication.password = "12345678910"};
    // 初始化mqtt客户端
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    // 注册任意的mqtt事件都使用同一个回调函数处理
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    // 启动mqtt
    esp_mqtt_client_start(client);

    return client;
}

// 订阅话题
esp_mqtt_client_subscribe(client, "topic1", 1);
// 发布话题
esp_mqtt_client_publish(client, "topic1", "topic1_data", 0, 0, 0);
```


# 杂项

## 事件循环机制

可以自定义事件来使用IO多路复用的模型，可以参考[例子](./example/others/eventloop.c)

```c
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_event_base.h"

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


// 发送数据到事件循环
esp_event_post_to(loop, TASK_EVENTS, TASK_ITERATION_EVENT, &iteration, sizeof(iteration), portMAX_DELAY);
```

## 控制台

通过可以argtable3库来创建终端程序，整个例子较长，参考[例子](./example/others/console.c)
