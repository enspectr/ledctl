#pragma once

#include <stdint.h>
#include <stdbool.h>

#define CONTROL_TOUT 150
#define CONTROL_TOUT_LONG 1500

typedef enum {
	evt_none,
	evt_released,
	evt_long_pressed
} control_evt_t;

struct control {
	bool          active;
	bool          active_long;
	uint32_t      last_active;
	uint32_t      active_ts;
	control_evt_t event;
};

// Update control state
static inline void control_up(struct control* ctl, bool active, uint32_t now)
{
	if (active) {
		ctl->last_active = now;
		if (!ctl->active) {
			ctl->active = true;
			ctl->active_ts = now;
		}
	}
	if (ctl->active) {
		if (now - ctl->last_active > CONTROL_TOUT) {
			bool const active_long = ctl->active_long;
			ctl->active = ctl->active_long = false;
			if (!active_long)
				ctl->event = evt_released;
		}
		else if (!ctl->active_long && now - ctl->active_ts > CONTROL_TOUT_LONG) {
			ctl->active_long = true;
			ctl->event = evt_long_pressed;
		}
	}
}

// Report last event. The particular event is reported only once.
static inline control_evt_t control_get_event(struct control* ctl)
{
	control_evt_t const evt = ctl->event;
	ctl->event = evt_none;
	return evt;
}
