/* LEAK.AOU: R12 resource probe.  Prints one line  LEAK heap=<free heap bytes> chans=<channel slots this job can still open>
 * spaces=<live paged address spaces> held=<paged bytes>; run it before and after a series of other programs and compare. */
#include <stdio.h>
#include "qdos386.h"

int main(void)
{
	int h[40], n = 0, i, c;
	struct qd_pginfo pi;
	unsigned int spaces = 9999, held = 9999;

	for (;;) {
		c = qd_open_ex("WIN1:/LEAK.AOU", QD_O_READ | QD_O_FILE);
		if (c < 0 || n >= 40)
			break;
		h[n++] = c;
	}
	for (i = 0; i < n; i++)
		qd_close(h[i]);
	if (qd_pginfo(0, &pi) == 0) {
		spaces = pi.spaces_live;
		held = pi.held_bytes;
	}
	printf("LEAK heap=%u chans=%d spaces=%u held=%u last=%d\n", qd_free_bytes(), n, spaces, held, c);
	return 0;
}
