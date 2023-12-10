#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "uart_events";

// 使用的是uart1
#define EX_UART_NUM UART_NUM_1
// 设置uart1的tx和rx分别对应的gpio3和gpio4
#define GPIO_TX 3
#define GPIO_RX 4
// 需要检测的关键字的长度
#define PATTERN_CHR_NUM (3)

// 接收缓冲区大小
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart_queue;


static void uart_event_task(void *pvParameters)
{
    /*
    event是串口回调的事件
    event.type是事件类型
    event.size是事件的数据长度
        而其中type又分为
            - UART_DATA：收到数据
            - UART_FIFO_OVF：硬件fifo溢出
            - UART_BUFFER_FULL：软件缓冲区满
            - UART_BREAK：收到break信号，一般是错误导致的
            - UART_PARITY_ERR：校验错误
            - UART_FRAME_ERR：帧错误
            - UART_PATTERN_DET：检测到特定字符
            - UART_EVENT_MAX：事件已满(？)
    */
    uart_event_t event;
    size_t buffered_size;
    uint8_t *dtmp = (uint8_t *)malloc(1024);
    for (;;)
    {
        // 接受数据
        if (xQueueReceive(uart_queue, (void *)&event, (TickType_t)portMAX_DELAY))
        {
            // 清空缓冲区
            bzero(dtmp, RD_BUF_SIZE);

            switch (event.type)
            {
            /*读取到数据
            打印事件与数据
            并写回串口
            */
            case UART_DATA:
                ESP_LOGI(TAG, "event - UART DATA: %s", (char *)dtmp);
                uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                uart_write_bytes(EX_UART_NUM, (const char *)dtmp, event.size);
                break;
            /*检测到对应的关键字：
            检测命令字符串末尾是否存在特定数量的相同字符
            如果发送abc+++123，那么由于+++是关键字
            所以先触发一条abc的UART_PATTERN_DET事件
            再触发一条123的UART_DATA事件
            */
            case UART_PATTERN_DET:
                uart_get_buffered_data_len(EX_UART_NUM, &buffered_size);
                // 获取关键字的位置
                int pos = uart_pattern_pop_pos(EX_UART_NUM);
                ESP_LOGI(TAG, "event - UART PATTERN DETECTED pos: %d, buffered size: %d", pos, buffered_size);
                if (pos == -1)
                {
                    // 这里表示关键字检测的缓冲区不够大，导致无法检测到关键字的位置
                    ESP_LOGW(TAG, "\tBuffer size is too small.");
                    uart_flush_input(EX_UART_NUM);
                }
                else
                {
                    uart_read_bytes(EX_UART_NUM, dtmp, pos, 100 / portTICK_PERIOD_MS);
                    uint8_t pat[PATTERN_CHR_NUM + 1];
                    memset(pat, 0, sizeof(pat));
                    uart_read_bytes(EX_UART_NUM, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
                    ESP_LOGI(TAG, "\tread data: %s", dtmp);
                    ESP_LOGI(TAG, "\tread pat : %s", pat);
                }
                break;
            /*FIFO溢出*/
            case UART_FIFO_OVF:
                ESP_LOGI(TAG, "hw fifo overflow");
                // 此时需要清空fifo
                uart_flush_input(EX_UART_NUM);
                xQueueReset(uart_queue);
                break;
            /*缓冲区满*/
            case UART_BUFFER_FULL:
                ESP_LOGI(TAG, "ring buffer full");
                // 此时直接清空缓冲区
                uart_flush_input(EX_UART_NUM);
                xQueueReset(uart_queue);
                break;
            /*rx break信号，一般是出错导致的*/
            case UART_BREAK:
                ESP_LOGI(TAG, "uart rx break");
                break;
            /*校验错误*/
            case UART_PARITY_ERR:
                ESP_LOGI(TAG, "uart parity error");
                break;
            /*帧错误*/
            case UART_FRAME_ERR:
                ESP_LOGI(TAG, "uart frame error");
                break;
            /*其他事件*/
            default:
                ESP_LOGI(TAG, "uart event type: %d", event.type);
                break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}

static void uart_intr_init()
{
    /*配置串口的帧格式*/
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    /*
    安装串口驱动
    第一个参数是uart端口号，
    第二个参数是接收缓冲区大小，
    第三个参数是发送缓冲区大小，
    第四个参数是队列大小，
    第五个参数是队列句柄，不需要为其手动分配空间，直接传入指针即可
    第六个参数是中断分配的优先级，不用管
    */
    uart_driver_install(EX_UART_NUM,
                        BUF_SIZE * 2,
                        BUF_SIZE * 2,
                        20,
                        &uart_queue,
                        0);

    /*串口初始化*/
    uart_param_config(EX_UART_NUM, &uart_config);

    /*设置串口引脚
    这里设置的是uart1的引脚，tx和rx分别设置为io3和io4
    */
    uart_set_pin(EX_UART_NUM, GPIO_TX, GPIO_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    /*这里设置串口检测到某条命令末尾特定字符重复特定次数就发起中断
    比如这里设置重复三个+号就设置对应的事件
    */
    uart_enable_pattern_det_baud_intr(
        /*串口号*/
        EX_UART_NUM,
        /*对应的检测字符，可以是字符也可以是字符串*/
        '+',
        /*字符重复多少次判定为一个事件*/
        PATTERN_CHR_NUM,
        /*关键字符间隔时间，单位为串口的时钟周期
        如果时间设置较大，比如大于一个字节时间则串口数据"+2+3+"也会被认为是关键字。
        */
        9,
        /*关键字触发后的有效时间，如果小于则不认为是最后一个关键字。
        如果有++++，则认为有4个+，不是关键字，如果时间设置较大，
        那+++后设置时间内务必不能再出现+，否则会认为是4个++++导致不触发中断
        */
        0,
        /*关键字触发前的间隔时间，如果小于则不认为是第一个关键字。
        同样，++++，如果时间设置较大，
        那+++之前设置时间内务必不能再出现+，否则会认为是4个++++导致不触发中断
         */
        0);

    /*用来检测关键字的队列长度*/
    uart_pattern_queue_reset(UART_NUM_1, 20);
}

void app_main(void)
{
    uart_intr_init();

    /*
    这里的串口会触发事件，需要创建一个任务来处理事件
    串口的中断被esp-idf更底层处理了
    而对应的中断会留一下事件标志，然后通过队列传递给任务
    可以根据触发的不同事件来做不同的回应
    */
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);
}
