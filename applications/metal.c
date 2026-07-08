#include "metal.h"
#include <rtdevice.h>

void metal_init(void)
{
    /* 设置为输入下拉模式，确保无信号时读取为低电平 */
    rt_pin_mode(METAL_DETECT_PIN, PIN_MODE_INPUT_PULLDOWN);
}

int metal_is_detected(void)
{
    int value = rt_pin_read(METAL_DETECT_PIN);
    return (value == PIN_HIGH) ? 1 : 0;
}
