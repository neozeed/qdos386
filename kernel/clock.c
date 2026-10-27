/*
 * clock.c - QDOS/386 wall clock arithmetic (R11F).  See clock.h.
 *
 * Pure C89, 32-bit unsigned arithmetic only (the historical compiler has no
 * long long).  No kernel dependencies.
 */
#include "clock.h"

void clock_set(struct clock_state *c, unsigned long anchor, unsigned long base_ticks,
               unsigned long num, unsigned long den, int pinned)
{
	c->anchor = anchor;
	c->base_ticks = base_ticks;
	c->num = num;
	c->den = den;
	c->acc = 0;
	c->secs = 0;
	/* Rejected: an anchor outside the supported range, a zero rate, or a tick
	 * so long that one tick alone could not be split into 32-bit chunks. */
	if (anchor > CLOCK_MAX_TIME || num == 0 || den == 0 || den > 0xFFFFFFFFUL - num) {
		c->state = CLOCK_NONE;
		return;
	}
	c->state = pinned ? CLOCK_PINNED : CLOCK_RUNNING;
}

void clock_advance(struct clock_state *c, unsigned long n)
{
	unsigned long chunk, m, tot, inc, limit;

	if (c->state != CLOCK_RUNNING)
		return;                       /* pinned, unset or overflowed: no advance */
	/* acc < num, so acc + m*den fits 32 bits when m*den <= 0xFFFFFFFF - num. */
	chunk = (0xFFFFFFFFUL - c->num) / c->den;
	limit = CLOCK_MAX_TIME - c->anchor;   /* the largest allowed secs */
	while (n > 0) {
		m = (n < chunk) ? n : chunk;
		tot = c->acc + m * c->den;
		inc = tot / c->num;
		if (inc > limit - c->secs) {
			c->state = CLOCK_OVERFLOW;
			return;
		}
		c->secs += inc;
		c->acc = tot % c->num;
		n -= m;
	}
}

unsigned long clock_now(const struct clock_state *c)
{
	if (c->state != CLOCK_RUNNING && c->state != CLOCK_PINNED)
		return CLOCK_UNAVAILABLE;
	return c->anchor + c->secs;
}
