/* memstat.c - R11L measurement build only (-DQD_MEMSTAT; never part of libc.a).  Reports at exit:
 *   heap peak (kernel heap bytes in use above the level at program start, including the kernel's
 *   per-block overhead), largest single block, allocation count, peak open channels, job stack peak
 *   (the stack is a 16 KiB heap block; a region below the entry frame is pre-filled with a pattern
 *   and scanned at exit), and elapsed timer ticks.
 * Host-independent: everything is measured inside the guest. */
#include <stdio.h>
#include <string.h>
#include "qdos386.h"

#define FILL_TOP   1024u         /* bytes below the entry stack pointer left unfilled */
#define FILL_SIZE  14336u        /* filled region (the stack block is 16384 bytes) */
#define PATTERN    0xA5A5A5A5ul

unsigned long qd_ms_top;         /* entry stack pointer, set by crt0_ms.s */
unsigned long qd_ms_free0, qd_ms_freemin, qd_ms_allocs, qd_ms_maxblk, qd_ms_live, qd_ms_livemax;
int qd_ms_open, qd_ms_openmax;
static unsigned long ticks0;

void qd_ms_begin(void)
{
	unsigned long *p;

	qd_ms_free0 = qd_ms_freemin = qd_free_bytes();
	ticks0 = qd_ticks();
	for (p = (unsigned long *) (qd_ms_top - FILL_TOP - FILL_SIZE);
	     p < (unsigned long *) (qd_ms_top - FILL_TOP); p++)
		*p = PATTERN;
}

void qd_ms_alloc(unsigned long n)
{
	unsigned long f = qd_free_bytes();

	qd_ms_allocs++;
	if (f < qd_ms_freemin)
		qd_ms_freemin = f;
	if (n > qd_ms_maxblk)
		qd_ms_maxblk = n;
}

void qd_ms_chan(int delta)
{
	qd_ms_open += delta;
	if (qd_ms_open > qd_ms_openmax)
		qd_ms_openmax = qd_ms_open;
}

void qd_ms_report(const char *who)
{
	unsigned long *p = (unsigned long *) (qd_ms_top - FILL_TOP - FILL_SIZE);
	unsigned long *e = (unsigned long *) (qd_ms_top - FILL_TOP);
	unsigned long peak;
	char buf[200];

	while (p < e && *p == PATTERN)
		p++;
	peak = (p == e) ? FILL_TOP : qd_ms_top - (unsigned long) p;
	sprintf(buf, "MEMSTAT %s heap_peak=%lu maxblk=%lu allocs=%lu chan_peak=%d stack_peak=%lu%s ticks=%lu\n",
	        who, qd_ms_free0 - qd_ms_freemin, qd_ms_maxblk, qd_ms_allocs, qd_ms_openmax, peak,
	        (p == (unsigned long *) (qd_ms_top - FILL_TOP - FILL_SIZE)) ? "+(overflow?)" : "",
	        qd_ticks() - ticks0);
	qd_flush_std();
	qd_write(qd_open("CON"), buf, (unsigned int) strlen(buf));
}
