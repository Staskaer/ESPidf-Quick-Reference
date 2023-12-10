#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

static const char *TAG = "UART TEST";

static void echo_task(void *arg)
{

    // 分配缓冲区
    uint8_t data[1024];

    while (1)
    {
        // 从串口中读取数据到缓冲区
        int len = uart_read_bytes(UART_NUM_1, &data, (1024 - 1), 20 / portTICK_PERIOD_MS);
        // 将缓冲区的数据写入到串口
        uart_write_bytes(UART_NUM_1, &data, len);
        // 打印读取到的数据到默认的串口(串口0)
        if (len)
        {
            data[len] = '\0';
            ESP_LOGI(TAG, "Recv str: %s", (char *)&data);
        }
    }
}

static void uart_init(void)
{

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
}

void app_main(void)
{
    uart_init();
    /*此时栈最好设置大一点*/
    xTaskCreate(echo_task, "uart_echo_task", 4096, NULL, 10, NULL);
}
