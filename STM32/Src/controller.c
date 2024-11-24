#include "main.h"
#include "controller.h"
#include "control.h"
#include "qenc.h"
#include "debug.h"
#include "util.h"
#include <stdint.h>

#define PWM_TIM htim3
#define CLK_TIM htim14

#define PWM_MAX 256
#define ENC_HIST_LEN 4
#define BOOST_MAX 16

static struct control ctl_btn;
static struct qenc    ctl_enc;
static int16_t        ctl_last_pos;
static int16_t        ctl_enc_hist[ENC_HIST_LEN];
static uint32_t       ctl_clock;
static uint32_t       ctl_sn;
static uint32_t       ctl_sn_processed;
static bool           ctl_on;
static uint16_t       ctl_pwm;

// Called every 500 usec
static void ctl_poll(void)
{
	++ctl_clock;
	control_up(&ctl_btn, !READ_PIN(ENC_BTN), ctl_clock/2);
	qenc_up(&ctl_enc, !READ_PIN(ENC_A), !READ_PIN(ENC_B));
	if (!(ctl_clock & 255))
		++ctl_sn;
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

static inline void ctl_set_pwm(uint16_t val)
{
	PWM_TIM.Instance->CCR1 = val;
}

void ctl_init(void)
{
	HAL_StatusTypeDef rc = HAL_TIM_Base_Start_IT(&CLK_TIM);
	BUG_ON(rc != HAL_OK);
	rc = HAL_TIM_PWM_Start(&PWM_TIM, TIM_CHANNEL_1);
	BUG_ON(rc != HAL_OK);
	// FIXME
	ctl_pwm = 128;
}

// Called every 128 msec
static void ctl_process(void)
{
	switch (control_get_event(&ctl_btn)) {
	case evt_released:
		ctl_on = !ctl_on;
		WRITE_PIN(nLED, !ctl_on);
		ctl_set_pwm(ctl_on ? ctl_pwm : 0);
		break;
	}

	int16_t boost = 1;
	for (int i = 0; i < ENC_HIST_LEN; ++i)
		boost += ctl_enc_hist[i];
	if (boost > BOOST_MAX)
		boost = BOOST_MAX;

	int16_t const pos = qenc_pos(&ctl_enc);
	int16_t delta = pos - ctl_last_pos;
	ctl_enc_hist[ctl_sn % ENC_HIST_LEN] = delta >= 0 ? delta : -delta;
	ctl_last_pos = pos;
	if (!ctl_on || !delta)
		return;

	int new_pwm = ctl_pwm + delta * boost;
	if (new_pwm < 0)
		new_pwm = 0;
	if (new_pwm > PWM_MAX)
		new_pwm = PWM_MAX;
	ctl_pwm = new_pwm;
	ctl_set_pwm(new_pwm);
}

void ctl_run(void)
{
	if (ctl_sn != ctl_sn_processed) {
		BUG_ON(ctl_sn_processed + 1 != ctl_sn);
		ctl_sn_processed = ctl_sn;
		ctl_process();
	}
}
