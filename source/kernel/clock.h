/*
 * clock.h - QDOS/386 wall clock (R11F).
 *
 * Model: wall time = anchor + elapsed seconds.  The anchor is an epoch value
 * (seconds since 1970-01-01 00:00:00 UTC) supplied at build time or at boot.
 * Elapsed seconds come from the timer interrupt: each tick adds the tick
 * length to an exact integer accumulator, so no rounding error builds up.
 * The rate is the platform's (clock_boot.c): for the PC, the PIT divisor gives
 * 1193182/11932 ticks per second (about 100.0017 Hz).
 *
 * States:
 *   CLOCK_NONE      no anchor configured: time is unavailable.  Reported, not
 *                   invented.  This is the default build.
 *   CLOCK_RUNNING   anchor configured, time advances with the timer.
 *   CLOCK_PINNED    anchor configured, elapsed time is held at zero.  For
 *                   reproducible test runs only (Phoon fixed-clock images).
 *   CLOCK_OVERFLOW  anchor + elapsed would exceed the largest valid time_t.
 *                   Sticky: the clock stops and reports unavailable.
 *
 * Valid time values are 0 .. CLOCK_MAX_TIME.  The value 0xFFFFFFFF is the
 * "unavailable" result of qd_wall_time() and time(), so it is never a time.
 *
 * This file has no kernel dependencies, so the same code is unit-tested on
 * the host (tests/host/r11f/clock_test.c).
 */
#ifndef QDOS386_CLOCK_H
#define QDOS386_CLOCK_H

#define CLOCK_MAX_TIME     0xFFFFFFFEUL
#define CLOCK_UNAVAILABLE  0xFFFFFFFFUL

#define CLOCK_NONE      0
#define CLOCK_RUNNING   1
#define CLOCK_PINNED    2
#define CLOCK_OVERFLOW  3

struct clock_state {
	unsigned long anchor;     /* epoch seconds at the baseline tick */
	unsigned long base_ticks; /* timer tick count at the baseline (low 32 bits) */
	unsigned long num;        /* ticks per second = num / den */
	unsigned long den;
	unsigned long acc;        /* 0 <= acc < num: fraction of a second, in units of 1/den ticks */
	unsigned long secs;       /* whole seconds elapsed since the baseline */
	int state;
};

/* Establish the baseline: anchor, tick count at that moment, and the rate. */
void clock_set(struct clock_state *c, unsigned long anchor, unsigned long base_ticks,
               unsigned long num, unsigned long den, int pinned);

/* Advance by n timer ticks.  Exact for any n: the work is split so that no
 * intermediate value exceeds 32 bits.  n may be the full 32-bit range. */
void clock_advance(struct clock_state *c, unsigned long n);

/* The current time, or CLOCK_UNAVAILABLE. */
unsigned long clock_now(const struct clock_state *c);

#endif
