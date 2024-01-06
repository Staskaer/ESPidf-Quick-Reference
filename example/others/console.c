#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
#include "nvs_flash.h"

/*
console基于argtable3库，所以最好配合argtable3的使用说明一起看

本程序实现了两个命令：echo和add
echo命令的使用方法：echo <str>会返回<str>
add命令的使用方法：add <a> <b>会返回a+b

可以使用help命令来查看注册的所有命令
*/

/*这里是两个错误检测，不支持usb cdc和usb serial jtag控制台，仅支持usb console控制台*/
#ifdef CONFIG_ESP_CONSOLE_USB_CDC
#error This example is incompatible with USB CDC console. Please try "console_usb" example instead.
#endif // CONFIG_ESP_CONSOLE_USB_CDC

#ifdef CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
#error This example is incompatible with USB serial JTAG console.
#endif // CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG

static const char *TAG = "example";
#define PROMPT_STR CONFIG_IDF_TARGET

/***************注册第一个echo命令***************/
static struct
{
    struct arg_str *echo;
    struct arg_end *end;
} echo_args;

/*执行echo命令的函数*/
int echo(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&echo_args);
    ESP_LOGI(TAG, "echo: %s", echo_args.echo->sval[0]);
    return 0;
}

/*注册echo命令*/
void register_echo(void)
{
    // 创建对应的argtable3的结构体，这里设置为str类型
    echo_args.echo = arg_str1(NULL, NULL, "<echo>", "Connection timeout, ms");
    echo_args.end = arg_end(20);

    // 注册此命令
    const esp_console_cmd_t join_cmd = {
        .command = "echo",
        .help = "print ",
        .hint = NULL,
        .func = &echo,
        .argtable = &echo_args};

    esp_console_cmd_register(&join_cmd);
}

/***************注册第二个add命令***************/

static struct
{
    struct arg_int *a;
    struct arg_int *b;
    struct arg_end *end;
} add_args;

int add(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&add_args);
    ESP_LOGI(TAG, "add: %d", add_args.a->ival[0] + add_args.b->ival[0]);
    return 0;
}

void register_add(void)
{
    add_args.a = arg_int1(NULL, NULL, "<a>", "First integer");
    add_args.b = arg_int1(NULL, NULL, "<b>", "Second integer");
    add_args.end = arg_end(2);

    const esp_console_cmd_t join_cmd = {
        .command = "add",
        .help = "Add two integers",
        .hint = NULL,
        .func = &add,
        .argtable = &add_args};

    esp_console_cmd_register(&join_cmd);
}

/***************初始化console***************/
static void initialize_console(void)
{
    /* Drain stdout before reconfiguring it */
    fflush(stdout);
    fsync(fileno(stdout));

    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    const uart_config_t uart_config = {
        .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
#if SOC_UART_SUPPORT_REF_TICK
        .source_clk = UART_SCLK_REF_TICK,
#elif SOC_UART_SUPPORT_XTAL_CLK
        .source_clk = UART_SCLK_XTAL,
#endif
    };
    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM,
                                        256, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config));

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {
        .max_cmdline_args = 8,
        .max_cmdline_length = 256,
#if CONFIG_LOG_COLORS
        .hint_color = atoi(LOG_COLOR_CYAN)
#endif
    };
    ESP_ERROR_CHECK(esp_console_init(&console_config));

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback *)&esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(100);

    /* Set command maximum length */
    linenoiseSetMaxLineLen(console_config.max_cmdline_length);

    /* Don't return empty lines */
    linenoiseAllowEmpty(false);

#if CONFIG_STORE_HISTORY
    /* Load command history from filesystem */
    linenoiseHistoryLoad(HISTORY_PATH);
#endif
}

void app_main(void)
{
    // 初始化nvs和console
    nvs_flash_init();
    initialize_console();

    // 注册help命令、echo命令和add命令
    esp_console_register_help_command();
    register_echo();
    register_add();

    // 设置颜色
    const char *prompt = LOG_COLOR_I PROMPT_STR "> " LOG_RESET_COLOR;

    printf("\n"
           "This is an example of ESP-IDF console component.\n"
           "Type 'help' to get the list of commands.\n"
           "Use UP/DOWN arrows to navigate through command history.\n"
           "Press TAB when typing command name to auto-complete.\n"
           "Press Enter or Ctrl+C will terminate the console environment.\n");

    // 如果终端不支持上下箭头调出历史命令就禁用这些功能
    int probe_status = linenoiseProbe();
    if (probe_status)
    {
        printf("\n"
               "Your terminal application does not support escape sequences.\n"
               "Line editing and history features are disabled.\n"
               "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
#if CONFIG_LOG_COLORS
        // 颜色也不支持
        prompt = PROMPT_STR "> ";
#endif
    }

    // 主循环，不断读取命令并解析
    while (true)
    {
        char *line = linenoise(prompt);
        if (line == NULL)
        {
            break;
        }
        // 如果命令长度大于零就加入到历史命令中
        if (strlen(line) > 0)
        {
            linenoiseHistoryAdd(line);
        }

        // 运行指令
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND)
        {
            printf("Unrecognized command\n");
        }
        else if (err == ESP_ERR_INVALID_ARG)
        {
            // 空指令
        }
        else if (err == ESP_OK && ret != ESP_OK)
        {
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
        }
        else if (err != ESP_OK)
        {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }
        // 释放内存
        linenoiseFree(line);
    }

    ESP_LOGE(TAG, "Error or end-of-input, terminating console");
    esp_console_deinit();
}
