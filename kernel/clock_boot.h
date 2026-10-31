/*
 * clock_boot.h - kernel wall-clock service (R11F).  See clock.h for the model.
 *
 * Build variants (chosen with -D on the kernel build, in I386FLAGS or PCFLAGS):
 *   (none)                           no anchor: wall time is unavailable.  Default.
 *   -DQDOS_CLOCK_EPOCH=<seconds>     anchor configured, time advances with ticks.
 *   -DQDOS_CLOCK_EPOCH=<seconds> -DQDOS_CLOCK_PINNED
 *                                    anchor configured, time held fixed.  For
 *                                    reproducible Phoon and time test runs.
 * If the timer is not started (QDOS_NO_TIMER builds) the clock is pinned, since
 * nothing would advance it.
 */
#ifndef QDOS386_CLOCK_BOOT_H
#define QDOS386_CLOCK_BOOT_H

/* Called once at boot, before timer_start.  timer_running: 1 if the timer
 * will be started, 0 if not. */
void clock_boot(int timer_running);

/* Called from the timer interrupt, once per tick. */
void clock_tick(void);

/* The current wall time (seconds since 1970-01-01 00:00:00 UTC), or
 * CLOCK_UNAVAILABLE (0xFFFFFFFF) when no clock is configured or it has
 * overflowed.  Backs the QT_WALL_TIME trap. */
unsigned long kernel_wall_time(void);

/* R13B: the anchor's source: 2 build-time QDOS_CLOCK_EPOCH, 1 the CMOS RTC (QDOS_R13B kernels), 0 none configured,
 * negative: the RTC_ERR_* (rtc.h) that left the clock unavailable. */
int clock_source(void);

#endif
