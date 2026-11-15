/* MAPTEST.AOU (R12A Gate 1): several copies of this one unchanged executable, all linked at 0x240000, run at once.
 *   usage: MAPTEST tag rounds status [q]      (q: no start lines, only the result line)
 * Each copy prints its directory (space), the frames that back its data word, its BSS counter and its stack, writes its
 * own tag into a global, then for `rounds` ticks increments a BSS counter and checks that its globals still hold its
 * own values (another copy writing the same virtual addresses must not be visible).  It ends with `status`.
 *   MAPTEST <tag> start t=<ticks> id=... space=<directory> origin=240000
 *   MAPTEST <tag> map <data va>><frame> <bss va>><frame> <stack va>><frame>
 *   MAPTEST <tag> PASS counter=N  /  MAPTEST <tag> FAIL ...                                                        */
#include <stdio.h>
#include <stdlib.h>
#include "qdos386.h"

static unsigned int marker = 0x11111111u;     /* .data */
static unsigned int counter;                  /* .bss  */
static char tagbuf[16];                       /* .bss  */

int main(int argc, char **argv)
{
	struct qd_procinfo pi;
	unsigned int want, i, rounds, bad = 0, local = 0;
	int status;
	unsigned long t0;

	if (argc < 4) {
		printf("usage: MAPTEST tag rounds status\n");
		return 1;
	}
	for (i = 0; argv[1][i] && i < sizeof tagbuf - 1; i++)
		tagbuf[i] = argv[1][i];
	rounds = (unsigned int) atoi(argv[2]);
	status = atoi(argv[3]);
	want = 0x01010101u * (unsigned int) (unsigned char) tagbuf[0];
	if (qd_proc_info(0, &pi) != 0) {
		printf("MAPTEST %s FAIL no process info (not an R12A kernel)\n", tagbuf);
		return 2;
	}
	marker = want;
	t0 = qd_ticks();
	if (argc < 5 || argv[4][0] != 'q') {
	printf("MAPTEST %s start t=%lu id=%lu space=%08lx origin=%06lx\n", tagbuf, t0, pi.id, pi.space, pi.origin);
	printf("MAPTEST %s map %06x>%08lx %06x>%08lx %08x>%08lx\n", tagbuf,
	       (unsigned int) &marker, qd_proc_va(&marker) & ~0xFFFul, (unsigned int) &counter,
	       qd_proc_va(&counter) & ~0xFFFul, (unsigned int) &local, qd_proc_va(&local) & ~0xFFFul);
	}
	for (i = 0; i < rounds; i++) {
		counter++;
		local++;
		qd_yield(1);
		if (marker != want || counter != i + 1u || local != i + 1u)
			bad++;
	}
	if (bad == 0 && tagbuf[0] == argv[1][0])
		printf("MAPTEST %s PASS t=%lu counter=%u marker=%08x exit %d space=%08lx\n", tagbuf, qd_ticks(), counter,
		       marker, status, pi.space);
	else
		printf("MAPTEST %s FAIL bad=%u marker=%08x counter=%u\n", tagbuf, bad, marker, counter);
	return bad ? 3 : status;
}
