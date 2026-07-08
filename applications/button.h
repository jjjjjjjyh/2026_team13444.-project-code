#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <rtthread.h>
#include <board.h>

#define PIN_BUTTON      GET_PIN(E, 4)   // 按钮接 PE4

void button_init(void);

#endif
