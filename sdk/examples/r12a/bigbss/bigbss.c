/* BIGBSS.AOU (R12A frame-exhaustion test): a 960 KiB BSS (240 frames) touched page by page, then a sleep.
 *   usage: BIGBSS ticks      prints  BIGBSS up id=... frames=...   and ends with status 0 */
#include <stdio.h>
#include <stdlib.h>
#include "qdos386.h"

static unsigned char big[0xF0000];

int main(int argc, char **argv)
{
	struct qd_procinfo pi;
	unsigned int i, sum = 0;
	long t = argc > 1 ? atol(argv[1]) : 100;

	for (i = 0; i < sizeof big; i += 4096u) {
		big[i] = (unsigned char) i;
		sum += big[i];
	}
	qd_proc_info(0, &pi);
	printf("BIGBSS up id=%lu frames=%lu pool_free=%lu sum=%u\n", pi.id, pi.frames, pi.pool_free, sum);
	qd_yield((unsigned int) t);
	return 0;
}
