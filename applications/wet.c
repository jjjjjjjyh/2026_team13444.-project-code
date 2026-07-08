#include "wet.h"
#include <rtdevice.h>
#include <rthw.h>

/* 对外接口：初始化 DHT11 引脚 */
void dht11_init(void)
{
    /* 设置引脚为开漏输出，默认释放总线（高电平） */
    rt_pin_mode(DHT11_PIN, PIN_MODE_OUTPUT_OD);
    rt_pin_write(DHT11_PIN, PIN_HIGH);
}

/* 读取 DHT11 数据（采用延时采样法，关中断保护） */
rt_err_t dht11_read(dht11_data_t *data)
{
    rt_uint8_t buf[5] = {0};
    rt_uint32_t i, j;

    if (data == RT_NULL)
        return RT_ERROR;

    /* ---------- 起始信号（允许被中断） ---------- */
    rt_pin_mode(DHT11_PIN, PIN_MODE_OUTPUT_OD);
    rt_pin_write(DHT11_PIN, PIN_LOW);
    rt_thread_mdelay(18);               // 至少 18ms
    rt_pin_write(DHT11_PIN, PIN_HIGH);
    rt_hw_us_delay(30);                 // 20-40us

    /* ---------- 以下时序必须关中断 ---------- */
    rt_base_t level = rt_hw_interrupt_disable();

    /* 切换为输入上拉（成品模块自带电阻，也可使用浮空输入） */
    rt_pin_mode(DHT11_PIN, PIN_MODE_INPUT_PULLUP);

    /* 等待 DHT11 响应低电平（约 80us） */
    rt_uint32_t timeout = 0;
    while (rt_pin_read(DHT11_PIN) == PIN_HIGH && timeout++ < 1000)
        rt_hw_us_delay(1);
    if (timeout >= 1000) goto error;

    /* 等待 DHT11 响应高电平（约 80us） */
    timeout = 0;
    while (rt_pin_read(DHT11_PIN) == PIN_LOW && timeout++ < 1000)
        rt_hw_us_delay(1);
    if (timeout >= 1000) goto error;

    /* 等待数据开始前的低电平（约 50us） */
    timeout = 0;
    while (rt_pin_read(DHT11_PIN) == PIN_HIGH && timeout++ < 1000)
        rt_hw_us_delay(1);
    if (timeout >= 1000) goto error;

    /* 读取 40 位数据（延时采样法） */
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 8; j++) {
            // 等待低电平结束（每个位开始的 50us 低电平）
            while (rt_pin_read(DHT11_PIN) == PIN_LOW);
            // 延时 40us，此时若数据位为 1 则引脚仍为高，为 0 则已变低
            rt_hw_us_delay(40);
            if (rt_pin_read(DHT11_PIN) == PIN_HIGH) {
                buf[i] |= (1 << (7 - j));
                // 等待剩余高电平结束，准备下一位的低电平
                while (rt_pin_read(DHT11_PIN) == PIN_HIGH);
            }
        }
    }

    rt_hw_interrupt_enable(level);

    /* 校验和 */
    if (buf[4] != (buf[0] + buf[1] + buf[2] + buf[3]))
        return RT_ERROR;

    /* 填充数据 */
    data->humidity_int    = buf[0];
    data->humidity_dec    = buf[1];
    data->temperature_int = buf[2];
    data->temperature_dec = buf[3];
    data->checksum        = buf[4];

    return RT_EOK;

error:
    rt_hw_interrupt_enable(level);
    return RT_ERROR;
}
