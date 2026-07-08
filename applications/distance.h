#ifndef __DISTANCE_H__
#define __DISTANCE_H__

#include <rtthread.h>
#include <board.h>          // 新增：提供 GET_PIN 宏

/* 超声波模块引脚配置（请根据实际接线修改） */
#define DISTANCE_TRIG_PIN      GET_PIN(A, 1)   // TRIG -> PA1
#define DISTANCE_ECHO_PIN      GET_PIN(G, 0)   // ECHO -> PA2

/* 最大等待回波时间（微秒），对应约 5 米距离 */
#define DISTANCE_MAX_ECHO_US   30000

void distance_init(void);
int distance_get_cm(void);

#endif
