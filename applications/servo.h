#ifndef __SERVO_H__
#define __SERVO_H__

#include <rtthread.h>

/* 舵机初始化（配置引脚、查找设备） */
int servo_init(void);

/* 设置舵机角度（单位：度，范围 0~180） */
void servo_set_angle(int angle);

#endif
