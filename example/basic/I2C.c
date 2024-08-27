#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "sdkconfig.h"

static const char *TAG = "i2c";

/*主机与从机的端口
把5-18 19-21连接起来，注意要连接的紧一点，杜邦线可能会有干扰，多试几次

注意，esp32c3只有1个i2c，只能和其他开发板配合使用
*/
#define MasterSDA GPIO_NUM_5
#define MasterCLK GPIO_NUM_18
#define SlaveSDA GPIO_NUM_19
#define SlaveCLK GPIO_NUM_21

/*主机i2c初始化*/
static void i2c_master_init(void)
{
    i2c_config_t conf = {
        /*i2c的主机模式*/
        .mode = I2C_MODE_MASTER,
        /*sda的管脚以及是否启用上拉，如果这里不启用，就要自己额外接一个上拉电阻*/
        .sda_io_num = MasterSDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        /*clk的管脚以及上拉*/
        .scl_io_num = MasterCLK,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        /*主时钟频率*/
        .master.clk_speed = 400000,
        /*选择时钟源
        I2C_SCLK_SRC_FLAG_FOR_NOMAL是自动选择时钟
        I2C_SCLK_SRC_FLAG_AWARE_DFS是选择REF参考时钟源，不随APB时钟源改变
        I2C_SCLK_SRC_FLAG_LIGHT_SLEEP是选择睡眠模式下的时钟
        */
        .clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL,
    };
    /*初始化i2c*/
    i2c_param_config(I2C_NUM_0, &conf);
    /*安装i2c的驱动
    第三第四个参数是从机的rx和tx的buffer，主机无效
    最后一个参数是中断相关，不用管
    */
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}

/*从机i2c初始化 */
static void i2c_slave_init(void)
{
    i2c_config_t conf_slave = {
        /*这里与主机一样*/
        .sda_io_num = SlaveSDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = SlaveCLK,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        /*模式配置为从机模式*/
        .mode = I2C_MODE_SLAVE,
        /*是否启用10位地址寻址*/
        .slave.addr_10bit_en = 0,
        /*设置从机的地址*/
        .slave.slave_addr = 0x28,
    };

    /*初始化*/
    i2c_param_config(I2C_NUM_1, &conf_slave);
    /*安装从机驱动
    这里设置接收发送的buffer
    */
    i2c_driver_install(I2C_NUM_1, I2C_MODE_SLAVE, 2 * 256, 2 * 256, 0);
}

void app_main(void)
{
    i2c_slave_init();
    i2c_master_init();

    uint8_t data[10] = {0, 1, 2, 3, 4};
    uint8_t SlaveData[10];

    while (1)
    {
        /*
        i2c_master_write_to_device这个函数是对整个i2c时序完成了封装

        如果想要使用更加底层的接口来直接管理i2c的时序，可以使用i2c_master_start，i2c_master_write_byte等函数
        有很多例程都是基于i2c_cmd_link_create等等函数来手动实现i2c的发送时序，但是完全没有必要这样做
        i2c_master_write_to_device内部就是基于这些函数来实现的
        */

        // 主机向指定地址发送指定长度的数据
        i2c_master_write_to_device(I2C_NUM_0, 0x28, data, 5, 1000 / portTICK_PERIOD_MS);
        // 从机接受的数据会直接进入到缓冲区中，这里直接从缓冲区里拿数据
        i2c_slave_read_buffer(I2C_NUM_1, SlaveData, 5, 1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "slave read: %d,%d,%d,%d,%d", SlaveData[0], SlaveData[1], SlaveData[2], SlaveData[3], SlaveData[4]);

        SlaveData[0] = 0xff;
        // 从机发送的数据也会进入到发送缓冲区，主机从缓冲区里拿数据
        i2c_slave_write_buffer(I2C_NUM_1, SlaveData, 5, 1000 / portTICK_PERIOD_MS);
        // 主机读取从机发送的数据
        i2c_master_read_from_device(I2C_NUM_0, 0x28, data, 5, 1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "master read: %d,%d,%d,%d,%d", data[0], data[1], data[2], data[3], data[4]);

        data[0] = 0;
        data[1] = 1;
        data[2] = 2;
        data[3] = 3;
        data[4] = 4;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}