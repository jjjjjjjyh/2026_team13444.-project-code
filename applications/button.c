#include "button.h"
#include "distance.h"
#include "metal.h"
#include "wet.h"
#include <rtdevice.h>
#include <rtthread.h>

#define DBG_TAG "button"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* 信号量句柄，用于同步按键事件 */
static rt_sem_t button_sem;

/* 外部中断回调函数（在中断上下文执行） */
static void button_irq_callback(void *args)
{
    /* 释放信号量，唤醒线程 */
    rt_sem_release(button_sem);
}

/* 按键处理线程入口 */
static void button_thread_entry(void *param)
{
    while (1)
    {
        /* 阻塞等待信号量，直到按键中断释放 */
        rt_sem_take(button_sem, RT_WAITING_FOREVER);

        /* ---- 以下为去抖和读取逻辑（在线程上下文，可延时） ---- */
        rt_thread_mdelay(20);   // 消抖延时
        if (rt_pin_read(PIN_BUTTON) == PIN_LOW)   // 确认按下
        {
            LOG_I("Button pressed, start sensor reading...");

            /* 执行一次完整的传感器读取（可调用 main 中原有逻辑） */
            sensor_read_once();   // 建议将此函数移至独立文件或在此调用外部函数

            /* 等待按键释放 */
            while (rt_pin_read(PIN_BUTTON) == PIN_LOW)
            {
                rt_thread_mdelay(10);
            }
            LOG_I("Button released.");
        }
    }
}

/* 初始化按键模块 */
void button_init(void)
{
    /* 1. 创建信号量，初始值为0 */
    button_sem = rt_sem_create("btn_sem", 0, RT_IPC_FLAG_FIFO);
    if (button_sem == RT_NULL)
    {
        LOG_E("Create button semaphore failed!");
        return;
    }

    /* 2. 配置 PE4 为中断模式（下降沿触发），并绑定回调 */
    rt_pin_mode(PIN_BUTTON, PIN_MODE_INPUT_PULLUP);
    rt_pin_attach_irq(PIN_BUTTON, PIN_IRQ_MODE_FALLING, button_irq_callback, RT_NULL);
    rt_pin_irq_enable(PIN_BUTTON, PIN_IRQ_ENABLE);

    /* 3. 创建按键处理线程，优先级可稍低于传感器读取线程（如 12） */
    rt_thread_t tid = rt_thread_create("btn_thd",
                                       button_thread_entry,
                                       RT_NULL,
                                       1024,
                                       12,
                                       10);
    if (tid != RT_NULL)
        rt_thread_startup(tid);
    else
        LOG_E("Create button thread failed!");
}
