#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_adc/adc_continuous.h"

#define _ADC_UNIT_STR(unit) #unit
#define ADC_UNIT_STR(unit) _ADC_UNIT_STR(unit)

/*
配置使用的ADC通道为ADC_UNIT_1
ADC_CONV_SINGLE_UNIT_1表示仅使用ADC1来转换
ADC_ATTEN_DB_11表示使用11db的衰减，也就是最大量程范围
ADC_BITWIDTH_12表示这里是12位
*/
#define ADC_UNIT ADC_UNIT_1
#define ADC_CONV_MODE ADC_CONV_SINGLE_UNIT_1
#define ADC_ATTEN ADC_ATTEN_DB_11
#define ADC_BIT_WIDTH ADC_BITWIDTH_12

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

#define ADC_READ_LEN 256

// 使用2和3两个通道
static adc_channel_t channel[2] = {ADC_CHANNEL_2, ADC_CHANNEL_3};

static TaskHandle_t s_task_handle;
static const char *TAG = "EXAMPLE";

// DMA的回调函数，是一个中断函数，作用是当DMA转换完成后，通知任务处理数据
static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    // 通知app_main线程继续处理
    vTaskNotifyGiveFromISR(s_task_handle, &mustYield);

    return (mustYield == pdTRUE);
}

// 初始化ADC-DMA转换
static void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle)
{
    adc_continuous_handle_t handle = NULL;

    /*
    配置ADC_DMA的最大存储缓冲区大小和每个转换帧大小
    */
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = ADC_READ_LEN,
    };
    adc_continuous_new_handle(&adc_config, &handle);

    /* 上面配置好了ADC_DMA的模式，下面来配置每一个ADC IO的采样配置
    sample_freq_hz：采样频率，这里是20KHz
    conv_mode：转换模式，这里是ADC_CONV_SINGLE_UNIT_1，也就是只使用ADC1
    format：DMA输出格式，这里是ADC_DMA_OUTPUT_TYPE，也就是type2
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
        .conv_mode = ADC_CONV_MODE,
        .format = ADC_DMA_OUTPUT_TYPE,
        .pattern_num = channel_num,
    };

    // 这里来配置adc_pattern，与adc单独使用时的参数一致
    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    for (int i = 0; i < channel_num; i++)
    {
        adc_pattern[i].atten = ADC_ATTEN;
        adc_pattern[i].channel = channel[i] & 0x7;
        adc_pattern[i].unit = ADC_UNIT;
        adc_pattern[i].bit_width = ADC_BIT_WIDTH;

        ESP_LOGI(TAG, "adc_pattern[%d].atten is :%" PRIx8, i, adc_pattern[i].atten);
        ESP_LOGI(TAG, "adc_pattern[%d].channel is :%" PRIx8, i, adc_pattern[i].channel);
        ESP_LOGI(TAG, "adc_pattern[%d].unit is :%" PRIx8, i, adc_pattern[i].unit);
    }
    // 然后完成配置并进行初始化
    dig_cfg.adc_pattern = adc_pattern;
    adc_continuous_config(handle, &dig_cfg);

    *out_handle = handle;
}

void app_main(void)
{
    esp_err_t ret;
    uint32_t ret_num = 0;

    // 存储数据
    uint8_t result[ADC_READ_LEN] = {0};
    memset(result, 0xcc, ADC_READ_LEN);

    // 获取当前的task的handler，后面用于接受dma的通知
    s_task_handle = xTaskGetCurrentTaskHandle();

    // 初始化ADC-DMA转换并获取handler
    adc_continuous_handle_t handle = NULL;
    continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &handle);

    // 注册DMA转换完成的回调函数
    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };
    adc_continuous_register_event_callbacks(handle, &cbs, NULL);

    // 启动adc_dma
    adc_continuous_start(handle);

    while (1)
    {
        // 阻塞当前线程，直到dma处理完时返回
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        char unit[] = ADC_UNIT_STR(ADC_UNIT);
        uint32_t data_total2 = 0, data_total3 = 0;
        while (1)
        {
            data_total2 = 0;
            data_total3 = 0;
            // 不断读取dma的数据
            ret = adc_continuous_read(handle, result, ADC_READ_LEN, &ret_num, 0);
            if (ret == ESP_OK)
            {
                ESP_LOGI(TAG, "ret is %x, ret_num is %" PRIu32 " bytes", ret, ret_num);
                for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES)
                {
                    // 获取每个dma传输后的数据、通道
                    adc_digi_output_data_t *p = (adc_digi_output_data_t *)&result[i];
                    uint32_t chan_num = ADC_GET_CHANNEL(p);
                    uint32_t data = ADC_GET_DATA(p);

                    // 然后根据通道号来累加数据
                    if (chan_num == 2)
                        data_total2 += data;
                    else
                        data_total3 += data;
                }

                // 打印平均值
                ESP_LOGI(TAG, "adc unit:%s", unit);
                ESP_LOGI(TAG, "\tdata_total2 is %ld, data_total3 is %ld", data_total2 / ret_num, data_total3 / ret_num);
                vTaskDelay(5000 / portTICK_PERIOD_MS);
            }
            else if (ret == ESP_ERR_TIMEOUT)
            {
                break;
            }
        }
    }

    // 停止adc dma并释放资源
    adc_continuous_stop(handle);
    adc_continuous_deinit(handle);
}
