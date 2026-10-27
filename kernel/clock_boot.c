/*
 * clock_boot.c - kernel wall-clock service (R11F).  See clock_boot.h.
 */
#include "clock.h"
#include "clock_boot.h"
#include "platform.h"
#include "jobs.h"

static struct clock_state kclock;

void clock_boot(int timer_running)
{
#ifdef QDOS_CLOCK_EPOCH
	unsigned long num, den;
	int pinned;

	timer_rate(&num, &den);
	pinned = 0;
# ifdef QDOS_CLOCK_PINNED
	pinned = 1;
# endif
	if (!timer_running)
		pinned = 1;
	/* The tick baseline is the tick count at boot; the timer has not started
	 * yet, so sched_ticks is 0 here, and the clock counts from that point. */
	clock_set(&kclock, (unsigned long) QDOS_CLOCK_EPOCH, sched_ticks,
	          num, den, pinned);
#else
	(void) timer_running;
	clock_set(&kclock, 0UL, 0UL, 0UL, 0UL, 0);   /* CLOCK_NONE */
#endif
}

void clock_tick(void)
{
	clock_advance(&kclock, 1UL);
}

unsigned long kernel_wall_time(void)
{
	return clock_now(&kclock);
}
