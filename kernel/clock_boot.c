/*
 * clock_boot.c - kernel wall-clock service (R11F).  See clock_boot.h.
 */
#include "clock.h"
#include "clock_boot.h"
#include "platform.h"
#include "jobs.h"

static struct clock_state kclock;
#ifdef QDOS_R13B
static int clock_src;
#endif

#if defined(QDOS_R13B) && !defined(QDOS_CLOCK_EPOCH) && !defined(QDOS_NO_RTC)
#include "rtc.h"
struct rtc_raw clock_raw;                    /* R13B: the registers read at boot (shown on the boot line) */
#endif

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
#elif defined(QDOS_R13B) && !defined(QDOS_NO_RTC)
	/* R13B: the CMOS RTC, read once now; from here the clock advances with the timer (wall time and scheduler ticks
	 * stay separate).  An RTC that cannot be read or decoded leaves the clock NONE: nothing is guessed. */
	unsigned long num, den, t = 0;
	int rc;

	timer_rate(&num, &den);
	rc = rtc_read(&t, &clock_raw);
	if (rc == RTC_OK) {
		clock_set(&kclock, t, sched_ticks, num, den, !timer_running);
		clock_src = 1;
	} else {
		clock_set(&kclock, 0UL, 0UL, 0UL, 0UL, 0);   /* CLOCK_NONE */
		clock_src = rc;
	}
#else
	(void) timer_running;
	clock_set(&kclock, 0UL, 0UL, 0UL, 0UL, 0);   /* CLOCK_NONE */
#endif
}

#ifdef QDOS_R13B
/* R13B: where the anchor came from - 2 build-time epoch, 1 CMOS RTC, 0 none configured, negative: the RTC_ERR_* that
 * left the clock unavailable. */
int clock_source(void)
{
#ifdef QDOS_CLOCK_EPOCH
	return 2;
#else
	return clock_src;
#endif
}
#endif

void clock_tick(void)
{
	clock_advance(&kclock, 1UL);
}

unsigned long kernel_wall_time(void)
{
	return clock_now(&kclock);
}
