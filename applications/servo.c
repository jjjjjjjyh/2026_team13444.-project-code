#include "servo.h"
#include "stm32f4xx_hal.h"

static TIM_HandleTypeDef htim4;

/* 配置 PD12 为 TIM4_CH1 复用，并初始化定时器 */
int servo_init(void)
{
    /* 使能时钟 */
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();

    /* 配置 GPIO */
    GPIO_InitTypeDef gpio = {
        .Pin = GPIO_PIN_12,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_HIGH,
        .Alternate = GPIO_AF2_TIM4
    };
    HAL_GPIO_Init(GPIOD, &gpio);

    /* 配置 TIM4 */
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 8400 - 1;      // 假设 APB1 时钟 84MHz，分频后 10kHz
    htim4.Init.Period = 200 - 1;          // 20ms（10kHz / 200 = 50Hz）
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&htim4);

    /* 配置 PWM 通道 1 */
    TIM_OC_InitTypeDef oc = {
        .OCMode = TIM_OCMODE_PWM1,
        .Pulse = 15,                      // 初始 1.5ms（10kHz 下计数值 15）
        .OCPolarity = TIM_OCPOLARITY_HIGH,
        .OCFastMode = TIM_OCFAST_DISABLE
    };
    HAL_TIM_PWM_ConfigChannel(&htim4, &oc, TIM_CHANNEL_1);

    /* 启动 PWM */
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);

    rt_kprintf("Servo init with HAL OK\n");
    return 0;
}

/* 设置角度，仅修改比较值，不重新初始化 */
void servo_set_angle(int angle)
{
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;

    /* 映射：0° -> 0.5ms (计数值 5)，180° -> 2.5ms (计数值 25) */
    uint32_t pulse = 5 + (angle * 20 / 180);   // 整数运算，注意调整
    // 如果希望更精确，可以用浮点，但这里用整数近似
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, pulse);
}
