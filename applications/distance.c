#include "distance.h"
#include <rtdevice.h>
#include <rthw.h>

void distance_init(void)
{
    rt_pin_mode(DISTANCE_TRIG_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(DISTANCE_ECHO_PIN, PIN_MODE_INPUT);
    rt_pin_write(DISTANCE_TRIG_PIN, PIN_LOW);
}

int distance_get_cm(void)
{
    int pulse_width = 0;
    int timeout = 0;

    /* 产生 10us 高电平触发脉冲 */
    rt_pin_write(DISTANCE_TRIG_PIN, PIN_LOW);
    rt_hw_us_delay(2);
    rt_pin_write(DISTANCE_TRIG_PIN, PIN_HIGH);
    rt_hw_us_delay(10);
    rt_pin_write(DISTANCE_TRIG_PIN, PIN_LOW);

    /* 等待 ECHO 上升沿（超时则返回 -1）*/
    timeout = 0;
    while (rt_pin_read(DISTANCE_ECHO_PIN) == PIN_LOW) {
        if (++timeout > DISTANCE_MAX_ECHO_US)
            return -1;
        rt_hw_us_delay(1);
    }

    /* 测量 ECHO 高电平宽度 */
    pulse_width = 0;
    while (rt_pin_read(DISTANCE_ECHO_PIN) == PIN_HIGH) {
        if (++pulse_width > DISTANCE_MAX_ECHO_US)
            return -1;
        rt_hw_us_delay(1);
    }

    /* HC-SR04 转换公式：距离(cm) = 高电平时间(us) / 58 */
    return pulse_width / 58;
}
