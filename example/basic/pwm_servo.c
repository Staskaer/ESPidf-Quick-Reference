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

// 通过角度计算占空比
int calculatePWM(int degree)
{
    // (角度/90+0.5)*1023/20
    // 如果不是10位占空比的话，需要把1023修改成对应的2^BIT - 1
    if (degree < 0)
        degree = 0;
    if (degree > 180)
        degree = 180;
    return (int)((degree / 90.0 + 0.5) * 1023 / 20.0);
}

/* 初始化PWM */
static void servo_init()
{
    /* 定时器配置 */
    static const ledc_timer_config_t servo_timer = {
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 50, // 50Hz的PWM频率，对应舵机20ms周期
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&servo_timer);

    /*通道配置，把通道与引脚匹配起来*/
    ledc_channel_config_t servo_channel = {
        .channel = LEDC_CHANNEL_0,         // LED使用通道0
        .duty = 5,                         // 占空比0
        .gpio_num = 2,                     // 信号IO
        .speed_mode = LEDC_LOW_SPEED_MODE, // 速率
        .hpoint = 0,                       // 没啥用，不用管
        .timer_sel = LEDC_TIMER_0,         // 使用上面初始化过的定时器
    };

    ledc_channel_config(&servo_channel);
}

// 控制舵机角度
void set_degree(uint8_t degree, uint8_t channel)
{
    int pwm = calculatePWM(degree);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, pwm);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
};

void app_main(void)
{
    // 初始化pwm
    servo_init();

    while (1)
    {
        // 控制舵机旋转
        for (int i = 0; i < 180; i += 10)
        {
            set_degree(i, 0);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}