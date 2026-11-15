/*
 * TIMEQ.AOU - R13B: the wall clock as a C program sees it (time(), gmtime(), ctime(), strftime()) next to the
 * scheduler tick count (qd_ticks), which is NOT a time of day.  usage: TIMEQ [wait_ticks]
 * Prints "TIMEQ t=<time_t> ticks=<n> utc=<YYYY-MM-DD HH:MM:SS> ctime=<...>"; with wait_ticks it sleeps that long and
 * prints a second line plus the differences.  No clock: "TIMEQ t=-1 unavailable".  Exit status 0, or 1 without a clock.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "qdos386.h"

static int show(const char *tag, time_t *out, unsigned long *tk)
{
	time_t t = time((time_t *) 0);
	unsigned long k = qd_ticks();
	struct tm *g;
	char b[40];

	*tk = k;
	*out = t;
	if (t == (time_t) -1) {
		printf("TIMEQ %s t=-1 unavailable ticks=%lu\n", tag, k);
		return 1;
	}
	g = gmtime(&t);
	if (g == 0 || strftime(b, sizeof b, "%Y-%m-%d %H:%M:%S", g) == 0)
		b[0] = 0;
	printf("TIMEQ %s t=%lu ticks=%lu utc=%s ctime=%s", tag, (unsigned long) t, k, b, ctime(&t));
	return 0;
}

int main(int argc, char **argv)
{
	time_t t1, t2;
	unsigned long k1, k2;
	long w = argc > 1 ? atol(argv[1]) : 0;
	int rc = show("now", &t1, &k1);

	if (w > 0) {
		qd_yield((unsigned int) w);
		rc |= show("after", &t2, &k2);
		if (rc == 0)
			printf("TIMEQ delta seconds=%lu ticks=%lu\n", (unsigned long) (t2 - t1), k2 - k1);
	}
	return rc;
}
