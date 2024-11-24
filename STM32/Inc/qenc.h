#pragma once

#include <stdint.h>
#include <stdbool.h>

#define QENC_FILTER 3

struct qenc {
	bool     lastA;
	bool     lastB;
	uint8_t  fltA;
	uint8_t  fltB;
	int32_t  cnt;
};

static inline void qenc_up(struct qenc* e, bool A, bool B)
{
	// State changed flags
	bool a = false, b = false;

	if (A != e->lastA) {
		if (++e->fltA > QENC_FILTER) {
			e->lastA = A;
			e->fltA = 0;
			a = true;
		} else {
			A = e->lastA;
		}
	} else
		e->fltA = 0;

	if (B != e->lastB) {
		if (++e->fltB >= QENC_FILTER) {
			e->lastB = B;
			e->fltB = 0;
			b = true;
		} else {
			B = e->lastB;
		}
	} else
		e->fltB = 0;

	int8_t const d = !A == !B ? 1 : -1;
	if (a)
		e->cnt += d;
	if (b)
		e->cnt -= d;
}

static inline int16_t qenc_pos(struct qenc* e)
{
	return (int16_t)((e->cnt + 2) / 4);
}

static inline void qenc_reset(struct qenc* e)
{
	e->cnt = 0;
}
