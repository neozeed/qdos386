/*
 * QDOS/386 R5 application 2: initialised data, zero BSS, stack use, and a
 * CPU-bound loop that is preempted by the timer (it never yields).
 * Returns 42 when every check passes, otherwise the mask of failed checks.
 */
#include "appcalls.h"

static int tbl[4] = { 3, 5, 7, 11 };      /* initialised data */
static char tag[] = "PREP";               /* initialised data */
static int zbss[64];                      /* BSS: must read as zero */
static volatile unsigned int spin;

#define SPIN_ITERS 2000000u

static int deep(int n)
{
	unsigned char local[64];
	int i;

	for (i = 0; i < 64; i++)
		local[i] = (unsigned char) (n + i);
	if (n > 0 && deep(n - 1) != 0)
		return 1;
	for (i = 0; i < 64; i++)
		if (local[i] != (unsigned char) (n + i))
			return 1;
	return 0;
}

int main(void)
{
	unsigned int bad = 0;
	unsigned int i;
	int sum = 0, ch;

	for (i = 0; i < 4; i++)
		sum += tbl[i];
	if (sum != 26)
		bad |= 1;
	if (tag[0] != 'P' || tag[3] != 'P')
		bad |= 2;
	for (i = 0; i < 64; i++)
		if (zbss[i] != 0)
			bad |= 4;
	zbss[10] = 99;
	if (zbss[10] != 99)
		bad |= 4;
	if (deep(12) != 0)
		bad |= 8;
	for (i = 0; i < SPIN_ITERS; i++)
		spin = spin + 1;
	if (spin != SPIN_ITERS)
		bad |= 16;

	ch = qtrap(APP_QT_CH_OPEN, (unsigned int) "CON", 0, 0);
	if (ch > 0) {
		if (bad == 0)
			qtrap(APP_QT_CH_WRITE, (unsigned int) ch, (unsigned int) "PREP: all checks ok\n", 20);
		qtrap(APP_QT_CH_CLOSE, (unsigned int) ch, 0, 0);
	}
	if (bad != 0)
		return (int) bad;
	return 42;
}
