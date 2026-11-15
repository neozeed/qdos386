/* OLDT.AOU (R12C Stage 2): a program linked the R12B way (tests/r12c/mk-oldt.sh: the R12B crt0, which does not call
 * qd_prog_title) that defines the C68 program name.  The R12C kernel finds _prog_name in the image's symbol table at
 * load time, so the task list shows "Old-Hack 0.9" although the binary was not built for R12C.
 *   usage: OLDT [ticks]   print one line, sleep, end 0.  Uses only interfaces older than R12C. */
#include <stdio.h>
#include <stdlib.h>
#include "qdos386.h"

char _prog_name[] = "Old-Hack 0.9";

int main(int argc, char **argv)
{
	long t = argc > 1 ? atol(argv[1]) : 0;

	printf("OLDT: linked without the R12C title call, sleeping %ld ticks\n", t);
	if (t > 0)
		qd_yield((unsigned int) t);
	return 0;
}
