#include "main.h"
#include "controller.h"
#include "debug.h"
#include <stdint.h>

#define PWM_TIM htim3
#define CLK_TIM htim14

static uint32_t ctl_clock;

static void ctl_poll(void)
{
	++ctl_clock;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim == &CLK_TIM)
		ctl_poll();
}

void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *htim)
{
	BUG();
}

static inline void ctl_set_pwm(uint8_t val)
{
	PWM_TIM.Instance->CCR1 = val + 1;
}

void ctl_init(void)
{
	HAL_StatusTypeDef rc = HAL_TIM_Base_Start_IT(&CLK_TIM);
	BUG_ON(rc != HAL_OK);
	rc = HAL_TIM_PWM_Start(&PWM_TIM, TIM_CHANNEL_1);
	BUG_ON(rc != HAL_OK);
}

void ctl_run(void)
{
}

