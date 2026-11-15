/* R11H MULTIB: CPU-bound work in a fixed-origin profile (slot 3, 0x500000).
 * Keeps writable globals and a canary of its own, prints progress with the tick
 * count, and exits with status 3.  Run concurrently with MULTIB by MULTI. */
#include <stdio.h>
#include "qdos386.h"

#define RUN_TICKS 80u

static unsigned long acc = 1ul;
static unsigned long canary[64];

int main(void)
{
	unsigned long start, last, n = 0ul;
	unsigned int i;

	for (i = 0; i < 64u; i++)
		canary[i] = 0x5B5B5B5Bul ^ (unsigned long) i;
	start = qd_ticks();
	last = start;
	while (qd_ticks() - start < RUN_TICKS) {
		acc = acc * 1664525ul + 1013904223ul;
		n++;
		if (qd_ticks() - last >= 10ul) {
			printf("B tick %u n %u\n", (unsigned int) qd_ticks(), (unsigned int) (n & 0xFFFFul));
			last = qd_ticks();
		}
	}
	for (i = 0; i < 64u; i++)
		if (canary[i] != (0x5B5B5B5Bul ^ (unsigned long) i))
			return 9;
	printf("B done acc %u\n", (unsigned int) (acc & 0xFFFFul));
	return 7;
}
