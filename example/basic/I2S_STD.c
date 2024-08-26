#include <string.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "sdkconfig.h"

static const char *TAG = "I2S";

/*
接线如下
4--15
2--13
14--12
16-0
*/
#define I2S1_STD_BCLK_IO1 GPIO_NUM_4
#define I2S1_STD_WS_IO1 GPIO_NUM_2
#define I2S1_STD_DOUT_IO1 GPIO_NUM_14
#define I2S1_STD_DIN_IO1 GPIO_NUM_16

#define I2S2_STD_BCLK_IO2 GPIO_NUM_15
#define I2S2_STD_WS_IO2 GPIO_NUM_13
#define I2S2_STD_DOUT_IO2 GPIO_NUM_0
#define I2S2_STD_DIN_IO2 GPIO_NUM_12

// I2S的缓冲区大小
#define I2S_BUFF_SIZE 2048

// i2s 的发送和接受handler
i2s_chan_handle_t tx_chan;
i2s_chan_handle_t rx_chan;

void i2s_init_std_simplex(void)
{
    // 使用std传输模式
    // I2S1是输出端主机，提供时钟信号
    // I2S2是接受端从机，只接收时钟信号
    i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    i2s_new_channel(&tx_chan_cfg, &tx_chan, NULL);
    i2s_chan_config_t rx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_SLAVE);
    i2s_new_channel(&rx_chan_cfg, NULL, &rx_chan);

    i2s_std_config_t tx_std_cfg = {
        // 时钟频率
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        // 16位数据、双声道
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S1_STD_BCLK_IO1,
            .ws = I2S1_STD_WS_IO1,
            .dout = I2S1_STD_DOUT_IO1,
            .din = I2S1_STD_DIN_IO1,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    // 初始化i2s1
    i2s_channel_init_std_mode(tx_chan, &tx_std_cfg);

    i2s_std_config_t rx_std_cfg = {
        // i2s2也是同样的配置
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        // 16位数据、双声道
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S2_STD_BCLK_IO2,
            .ws = I2S2_STD_WS_IO2,
            .dout = I2S2_STD_DOUT_IO2,
            .din = I2S2_STD_DIN_IO2,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    // 双声道传输
    // 这里需要指定接收端同时接收两个声道的数据
    // 因为i2s不同声道的数据对应的ws是不同的
    // 所以需要指定是全部接收还是只接收某个声道数据
    rx_std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;
    i2s_channel_init_std_mode(rx_chan, &rx_std_cfg);
}

// i2s发送任务
void i2s_send_task(void *args)
{
    uint8_t *w_buf = (uint8_t *)calloc(1, I2S_BUFF_SIZE);
    assert(w_buf);

    for (int i = 0; i < I2S_BUFF_SIZE; i += 1)
    {
        w_buf[i] = (uint8_t)i & 0xff;
    }

    size_t w_bytes = 0;
    vTaskDelay(pdMS_TO_TICKS(200));
    while (1)
    {
        i2s_channel_enable(tx_chan);
        if (i2s_channel_write(tx_chan, w_buf, I2S_BUFF_SIZE, &w_bytes, 1000) == ESP_OK)
        {
        }
        else
            printf("Write Task: i2s write failed\n");
        i2s_channel_disable(tx_chan);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    free(w_buf);
    vTaskDelete(NULL);
}

void i2s_read_task(void *args)
{
    uint8_t *r_buf = (uint8_t *)calloc(1, I2S_BUFF_SIZE);
    assert(r_buf);
    size_t r_bytes = 0;

    while (1)
    {
        /*这里是esp32接收到i2s数据
         */
        if (i2s_channel_read(rx_chan, r_buf, I2S_BUFF_SIZE, &r_bytes, 2500) == ESP_OK)
            ESP_LOGI(TAG, "Read Task: i2s read %d bytes: \n%d,%d,%d,%d\n%d,%d,%d,%d", r_bytes, r_buf[0], r_buf[1], r_buf[2], r_buf[3], r_buf[4], r_buf[5], r_buf[6], r_buf[7]);
        else
            printf("Read Task: i2s read failed\n");
    }
    free(r_buf);
    vTaskDelete(NULL);
}

void app_main(void)
{
    /*i2s初始化 */
    i2s_init_std_simplex();
    i2s_channel_enable(rx_chan);
    i2s_channel_enable(tx_chan);

    xTaskCreate(i2s_send_task, "i2s_send_task", 4096, NULL, 5, NULL);
    xTaskCreate(i2s_read_task, "i2s_read_task", 4096, NULL, 5, NULL);
}