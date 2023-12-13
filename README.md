# 这是什么？

目前用了到ESP-IDF，但是API不太好记，而且官方例程很长，所以就基于例程把大部分常用的API提取出来，方便自己查阅。

所有例程都在`example`目录下，单文件且仅保留最少的必要代码，并给出了API的必要的注释。

# 与官方文档示例有什么区别？

官方文档罗列出了所有的API片段，仅参考文档经常容易产生：**我应该用哪个API？只用这个API就够了吗？需不需要其他的操作**？的挠头疑惑

而官方示例则过于具体，参照示例时容易发出：**实现这个功能我应该抄哪些部分**？的灵魂拷问

这里只给出实现xxx功能的**单文件最小实现的示例并包含常用操作**，copy的时候直接整段复制就好了。同时示例中包含完整的API文档，常用操作省去了翻文档的时间。

# 会针对相关API来解释对应硬件原理或设计嘛？

不会，本质就是一个带了完整例程的快速参考手册。

---

# 目录


* [常用操作](./Reference.md#常用操作)
  * [串口日志输出](./Reference.md#串口日志输出)
  * [延时](./Reference.md#延时)
* [FreeRTOS部分](./Reference.md#freertos部分)
  * [FreeRTOS任务](./Reference.md#freertos任务)
    * [不指定CPU创建任务(FreeRtos)](./Reference.md#不指定cpu创建任务freertos)
    * [指定CPU创建任务](./Reference.md#指定cpu创建任务)
  * [FreeRTOS队列](./Reference.md#freertos队列)
    * [任务之间的队列](./Reference.md#任务之间的队列)
    * [中断函数之间的队列](./Reference.md#中断函数之间的队列)
  * [FreeRTOS定时器](./Reference.md#freertos定时器)
  * [信号量](./Reference.md#信号量)
    * [信号量/互斥量](./Reference.md#信号量互斥量)
    * [计数信号量](./Reference.md#计数信号量)
* [基本外设](./Reference.md#基本外设)
  * [GPIO](./Reference.md#gpio)
    * [初始化](./Reference.md#初始化)
    * [设置电平](./Reference.md#设置电平)
    * [读取电平](./Reference.md#读取电平)
  * [GPIO中断](./Reference.md#gpio中断)
    * [开启中断](./Reference.md#开启中断)
    * [中断回调函数与中断处理](./Reference.md#中断回调函数与中断处理)
  * [定时器](./Reference.md#定时器)
    * [硬件通用定时器](./Reference.md#硬件通用定时器)
    * [ESP定时器](./Reference.md#esp定时器)
  * [pwm](./Reference.md#pwm)
    * [生成两路pwm](./Reference.md#生成两路pwm)
    * [pwm控制sg90舵机](./Reference.md#pwm控制sg90舵机)
  * [UART](./Reference.md#uart)
    * [直接使用串口](./Reference.md#直接使用串口)
    * [通过事件来使用串口](./Reference.md#通过事件来使用串口)
* [无线通信](./Reference.md#无线通信)
  * [WIFI](./Reference.md#wifi)
    * [AP](./Reference.md#ap)
    * [STA](./Reference.md#sta)
    * [STA扫描](./Reference.md#sta扫描)
    * [STA静态IP与DNS](./Reference.md#sta静态ip与dns)