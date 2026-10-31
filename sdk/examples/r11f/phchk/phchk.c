/* PHCHK: resource probe for the repeated-run test (R11F).  Run from the shell
 * between Phoon runs.  It prints the free heap, the heap invariant result, and
 * the id of the first channel it can open (a leaked channel would move that id
 * up on every run).  Each value must match the baseline printed by the first
 * PHCHK run; the log comparison is in scripts/r11f-run-all.sh. */
#include <stdio.h>
#include "../check.h"
#include "qdos386.h"

int main(void)
{
	int ch;

	printf("PHCHK free heap %u\n", qd_free_bytes());
	printf("PHCHK heap invariants %d\n", qd_heap_ok());
	ch = qd_open("WIN1_LINES.TXT");
	printf("PHCHK first channel %d\n", ch);
	if (ch > 0)
		qd_close(ch);
	CHECK("heap invariants hold", qd_heap_ok() == 1);
	CHECK("a channel opens", ch > 0);
	return finish("PHCHK");
}
