#pragma once

#include "periodic_timer.h"

periodic_timer_t timer_1sec;

static inline void periodic_timers_init(void)
{
    periodic_timer_init(&timer_1sec, 1000);
}
