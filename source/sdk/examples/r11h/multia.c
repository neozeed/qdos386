/* R11H MULTIA: CPU-bound work in a fixed-origin profile (slot 2, 0x380000).
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
		canary[i] = 0xA0A0A0A0ul ^ (unsigned long) i;
	start = qd_ticks();
	last = start;
	while (qd_ticks() - start < RUN_TICKS) {
		acc = acc * 1103515245ul + 12345ul;
		n++;
		if (qd_ticks() - last >= 10ul) {
			printf("A tick %u n %u\n", (unsigned int) qd_ticks(), (unsigned int) (n & 0xFFFFul));
			last = qd_ticks();
		}
	}
	for (i = 0; i < 64u; i++)
		if (canary[i] != (0xA0A0A0A0ul ^ (unsigned long) i))
			return 9;
	printf("A done acc %u\n", (unsigned int) (acc & 0xFFFFul));
	return 3;
}
