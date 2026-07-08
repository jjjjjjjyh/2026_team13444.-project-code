#ifndef __METAL_H__
#define __METAL_H__

#include <rtthread.h>
#include <board.h>          // 新增

#define METAL_DETECT_PIN       GET_PIN(E, 11)

void metal_init(void);
int metal_is_detected(void);

#endif
