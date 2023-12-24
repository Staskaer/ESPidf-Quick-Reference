#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

const static char *TAG = "EXAMPLE";

/*
使用的两个ADC channel和对应的衰减
衰减见app_main中的注释
 */
#define EXAMPLE_ADC1_CHAN0 ADC_CHANNEL_2
#define EXAMPLE_ADC1_CHAN1 ADC_CHANNEL_3
#define EXAMPLE_ADC_ATTEN ADC_ATTEN_DB_11

static int adc_raw[2][10];
static int voltage[2][10];

/*
这个函数负责ADC的校准，校准被烧录在efuse中，存在曲线拟合校准和线性校准
曲线拟合的效果会更好，所以首先会尝试使用曲线校准，如果不支持曲线校准，那么再去使用直线校准

这个函数的输入为ADC和对应衰减，返回一个校准后的handler
*/
static bool adc_calibration(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    // 首先查看支持的校准方案
    // 0x0表示仅支持直线校准，0x2表示同时支持曲线和直线校准
    adc_cali_scheme_ver_t scheme_mask = 0;
    adc_cali_check_scheme(&scheme_mask);
    ESP_LOGI(TAG, "supported calibration scheme: 0x%x (0 for line, 2 for curve)", scheme_mask);

    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

// 曲线拟合校准
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated)
    {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        // 配置曲线拟合校准的参数并校准
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }
#endif

// 如果不支持曲线校准再使用直线校准，如果上面校准成功了，那么下面代码不会执行
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated)
    {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }
#endif

    // 返回校准结果
    *out_handle = handle;
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Calibration Success");
    }
    else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated)
    {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    }
    else
    {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

/*不需要用到adc时调用，主要是去释放校准的的handler的空间*/
static void adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}

void app_main(void)
{
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

        这里EXAMPLE_ADC_ATTEN通过宏配置成了ADC_ATTEN_DB_11，也就是最大量程
        */
        .atten = EXAMPLE_ADC_ATTEN,
    };

    // 初始化两个通道，分别是2通道和3通道
    adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN0, &config);
    adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN1, &config);

    /*下面进行ADC校准，校准仅与衰减和adc有关*/
    adc_cali_handle_t adc1_cali_chan0_handle = NULL;
    adc_cali_handle_t adc1_cali_chan1_handle = NULL;
    bool do_calibration1_chan0 = adc_calibration(ADC_UNIT_1, EXAMPLE_ADC_ATTEN, &adc1_cali_chan0_handle);
    bool do_calibration1_chan1 = adc_calibration(ADC_UNIT_1, EXAMPLE_ADC_ATTEN, &adc1_cali_chan1_handle);

    /*使用ADC进行不断读取*/
    while (1)
    {
        // 读取对应的通道的ADC值
        adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN0, &adc_raw[0][0]);
        ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN0, adc_raw[0][0]);
        // 读取对应通道的校准后的电压值
        if (do_calibration1_chan0)
        {
            adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_raw[0][0], &voltage[0][0]);
            ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN0, voltage[0][0]);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));

        // 另一个通道也是读取校准前后的电压值并打印
        adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN1, &adc_raw[0][1]);
        ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN1, adc_raw[0][1]);
        if (do_calibration1_chan1)
        {
            adc_cali_raw_to_voltage(adc1_cali_chan1_handle, adc_raw[0][1], &voltage[0][1]);
            ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN1, voltage[0][1]);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // 释放资源的操作
    adc_oneshot_del_unit(adc1_handle);
    if (do_calibration1_chan0)
    {
        adc_calibration_deinit(adc1_cali_chan0_handle);
    }
    if (do_calibration1_chan1)
    {
        adc_calibration_deinit(adc1_cali_chan1_handle);
    }
}
