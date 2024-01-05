#include <stdio.h>
#include "driver/dac.h"
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

/*
直接使用DAC输出，配置时只需要指定对应的通道即可
*/
void dac_init(void)
{
    dac_output_enable(DAC_CHANNEL_2);
}

/*
使用DAC的正弦信号发生器，配置时需要指定通道、频率、幅度、相位
*/
void dac_cw_init(void)
{
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
}

void app_main(void)
{
    // 初始化DAC
    dac_init();
    dac_cw_init();

    // 不断写dac2的数据
    while (1)
    {
        for (int i = 0; i < 256; i++)
        {
            // 设置dac2的电压
            dac_output_voltage(DAC_CHANNEL_2, i);
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }

    // 关闭dac的通道和正弦发生器
    dac_output_disable(DAC_CHANNEL_2);
    dac_cw_generator_disable();
    dac_output_disable(DAC_CHANNEL_1);
}
