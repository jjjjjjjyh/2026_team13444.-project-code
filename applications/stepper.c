#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <rthw.h>          // 提供 rt_hw_us_delay
#include "stepper.h"

/* TMC2209 控制引脚（与之前定义一致） */
#define PIN_STEP    GET_PIN(F, 7)
#define PIN_DIR     GET_PIN(F, 6)
#define PIN_MS1     GET_PIN(F, 5)
#define PIN_MS2     GET_PIN(F, 4)
#define PIN_EN      GET_PIN(D, 11)


#define STEPS_PER_90        810
#define FULL_CIRCLE_STEPS   (4 * STEPS_PER_90)

/* 当前绝对步数位置（假设上电时在 0° 位置） */
static volatile int current_abs_steps = 0;

/**
 * @brief 内部函数：旋转指定步数（正数顺时针，负数逆时针）
 * @param steps 步数（正负表示方向）
 */
static void stepper_rotate_steps(int steps)
{
    if (steps == 0) return;

    /* 设置方向 */
    if (steps > 0) {
        rt_pin_write(PIN_DIR, PIN_HIGH);   // 假设高电平顺时针
    } else {
        rt_pin_write(PIN_DIR, PIN_LOW);    // 低电平逆时针
        steps = -steps;
    }

    /* 使能电机（低电平使能） */
    rt_pin_write(PIN_EN, PIN_LOW);

    /* 产生脉冲，周期 1ms（500us 高 + 500us 低） */
    for (int i = 0; i < steps; i++) {
        rt_pin_write(PIN_STEP, PIN_HIGH);
        rt_hw_us_delay(2000);
        rt_pin_write(PIN_STEP, PIN_LOW);
        rt_hw_us_delay(2000);
    }

    /* 更新当前位置 */
    if (rt_pin_read(PIN_DIR) == PIN_HIGH) {
        current_abs_steps += steps;
    } else {
        current_abs_steps -= steps;
    }

    /* 保持使能，以锁定位置（也可根据需要失能，但会丢失位置） */
    // rt_pin_write(PIN_EN, PIN_HIGH);   // 不推荐失能
}

/**
 * @brief 初始化引脚，配置微步细分，使能驱动
 */
void stepper_init(void)
{
    rt_pin_mode(PIN_STEP, PIN_MODE_OUTPUT);
    rt_pin_mode(PIN_DIR,  PIN_MODE_OUTPUT);
    rt_pin_mode(PIN_MS1,  PIN_MODE_OUTPUT);
    rt_pin_mode(PIN_MS2,  PIN_MODE_OUTPUT);
    rt_pin_mode(PIN_EN,   PIN_MODE_OUTPUT);

    /* 初始电平 */
    rt_pin_write(PIN_EN,   PIN_LOW);   // 使能驱动
    rt_pin_write(PIN_MS1,  PIN_HIGH);  // 1/8 微步（可按需调整）
    rt_pin_write(PIN_MS2,  PIN_HIGH);
    rt_pin_write(PIN_DIR,  PIN_HIGH);
    rt_pin_write(PIN_STEP, PIN_LOW);

    current_abs_steps = 0;   // 假设初始位置为 0°
}

/**
 * @brief 移动到目标垃圾桶位置（自动计算最短路径）
 * @param target_bin 目标垃圾桶编号
 */
void stepper_move_to(bin_id_t target_bin)
{
    int target_steps = target_bin * STEPS_PER_90;
    int delta = target_steps - current_abs_steps;

    /* 计算最短路径（限制在 ±半圈 内） */
    while (delta > FULL_CIRCLE_STEPS / 2)
        delta -= FULL_CIRCLE_STEPS;
    while (delta < -FULL_CIRCLE_STEPS / 2)
        delta += FULL_CIRCLE_STEPS;

    if (delta != 0) {
        stepper_rotate_steps(delta);
        rt_kprintf("Stepper moved to bin %d, current steps=%d\n", target_bin, current_abs_steps);
    } else {
        rt_kprintf("Already at target bin %d\n", target_bin);
    }
}
