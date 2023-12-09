#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include <esp_err.h>
#include <driver/gpio.h>
#include <driver/ledc.h>

static const char *TAG = "Main";

/* 初始化PWM */
static void lcd_bl_init()
{
    /* 定时器配置 */
    static const ledc_timer_config_t lcd_bl_ledc_timer = {
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

    /*通道配置，把通道与引脚匹配起来*/
    // 初始化通道0，使用通道0对应3号引脚
    ledc_channel_config_t lcd_bl_ledc_channel0 = {
        .channel = LEDC_CHANNEL_0,         // LED使用通道0
        .duty = 0,                         // 占空比0
        .gpio_num = 3,                     // 连接LED的IO
        .speed_mode = LEDC_LOW_SPEED_MODE, // 速率
        .hpoint = 0,                       // 没啥用，不用管
        .timer_sel = LEDC_TIMER_0,         // 使用上面初始化过的定时器
    };
    ledc_channel_config(&lcd_bl_ledc_channel0);

    // 再初始化一下通道1，使用通道一对应4号引脚
    lcd_bl_ledc_channel0.channel = LEDC_CHANNEL_1;
    lcd_bl_ledc_channel0.gpio_num = 4;
    ledc_channel_config(&lcd_bl_ledc_channel0);
}

void lcd_bl_set(uint8_t brightness, uint8_t channel)
{
    /* 设定PWM占空比 */
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, brightness * 10);

    /* 更新PWM占空比输出 */
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

void app_main(void)
{
    // 初始化pwm
    lcd_bl_init();

    while (1)
    {
        // 不断改变占空比
        for (int i = 0; i < 80; i++)
        {
            lcd_bl_set(i, 0);
            lcd_bl_set(i, 1);
            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
        for (int i = 80; i > 0; i--)
        {
            lcd_bl_set(i, 0);
            lcd_bl_set(i, 1);
            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
    }
}