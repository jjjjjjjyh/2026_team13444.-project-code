#ifndef APPLICATIONS_STEPPER_H_
#define APPLICATIONS_STEPPER_H_

#include <rtthread.h>

/* 垃圾桶编号（对应 0°, 90°, 180°, 270°） */
typedef enum {
    BIN_RECYCLABLE = 0,   // 可回收 -> 0°
    BIN_HAZARDOUS  = 1,   // 有害    -> 90°
    BIN_KITCHEN    = 2,   // 厨余    -> 180°
    BIN_OTHER      = 3    // 其他    -> 270°
} bin_id_t;

void stepper_init(void);
void stepper_move_to(bin_id_t target_bin);

#endif /* APPLICATIONS_STEPPER_H_ */
