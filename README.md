# 这是什么？

目前用了到esp-idf，但是API不太好记，而且官方例程很长，所以就基于例程把大部分常用的API提取出来，方便自己查阅。

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
