/* R11K PGRUN - the generic launcher for paged programs (kernels built with -DQDOS_PAGING).
 *
 *   PGRUN copies program [args...]
 *     copies   1..3 simultaneous copies of the same WIN1_ root file
 *     program  a standard slot-1 a.out (linked once at 0x240000, the SDK default)
 *     args     passed unchanged to every copy as argv[1]...
 *
 * All copies are started HELD, so each owns its address space before any runs; if any start fails
 * the ones already made are cancelled and nothing runs (all or nothing).  Then every copy is
 * activated in one go, PGRUN waits for all of them, and reports.  Each copy has its own 1 MiB image
 * window; the heap, stacks and console are shared (docs/R11K-PAGING-APP-ABI.md).
 *
 * Output lines (machine-readable):
 *   PGRUN: start <i> id <id> space <space>       one per copy
 *   PGRUN: overlap yes|no (<n> samples)          both/all copies alive and dispatched at one instant
 *   PGRUN: exit <i> status <hex>                 the program's exit status (or a QD_ERR_*)
 *   PGRUN: released yes|no                       address spaces and frames back to the baseline
 *   PGRUN: DONE | PGRUN: FAILED <why>
 * Exit status 0 only if every copy was started, ended, and was reclaimed. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qdos386.h"

#define MAXC 3

int main(int argc, char **argv)
{
	int copies, ids[MAXC], i, started = 0, ok = 1, overlap = 0, samples = 0, st[MAXC];
	static char packed[512];
	unsigned int plen = 0, n;
	struct qd_pginfo base, in[MAXC], after;
	int alive;

	if (argc < 3) {
		printf("usage: PGRUN copies program [args...]\n");
		return 2;
	}
	copies = atoi(argv[1]);
	if (copies < 1 || copies > MAXC) {
		printf("PGRUN: FAILED copies must be 1..%d\n", MAXC);
		return 2;
	}
	if (!qd_pg_available()) {
		printf("PGRUN: FAILED this kernel has no paging (build with -DQDOS_PAGING)\n");
		return 3;
	}
	for (i = 3; i < argc; i++) {
		n = (unsigned int) strlen(argv[i]) + 1;
		if (plen + n > sizeof packed) {
			printf("PGRUN: FAILED arguments too long\n");
			return 2;
		}
		memcpy(packed + plen, argv[i], n);
		plen += n;
	}
	if (qd_pginfo(0, &base) != 0) {
		printf("PGRUN: FAILED cannot read the baseline\n");
		return 3;
	}
	for (i = 0; i < copies; i++) {
		ids[i] = qd_pg_startv(argv[2], packed, plen, QD_PG_HELD);
		if (ids[i] <= 0) {
			printf("PGRUN: FAILED start of copy %d: error %d\n", i + 1, ids[i]);
			while (--i >= 0)
				qd_pg_cancel(ids[i]);
			return 4;
		}
		started++;
		if (qd_pginfo(ids[i], &in[i]) != 0) {
			printf("PGRUN: FAILED info of copy %d\n", i + 1);
			for (; i >= 0; i--)
				qd_pg_cancel(ids[i]);
			return 4;
		}
		printf("PGRUN: start %d id %d space %05x\n", i + 1, ids[i], in[i].space);
	}
	for (i = 0; i < copies; i++)
		if (qd_job_activate(ids[i], 2) != 0) {
			printf("PGRUN: activate %d failed\n", i + 1);
			ok = 0;
		}
	do {
		alive = 0;
		for (i = 0; i < copies; i++)
			if (qd_job_exit_status(ids[i]) == QD_ERR_RUNNING)
				alive++;
		if (alive == copies) {
			int all = 1;
			for (i = 0; i < copies; i++)
				if (qd_pginfo(ids[i], &in[i]) != 0 || in[i].dispatches == 0)
					all = 0;
			samples++;
			if (all)
				overlap = 1;
		}
		if (alive)
			qd_yield(1);
	} while (alive);
	printf("PGRUN: overlap %s (%d samples)\n", overlap ? "yes" : "no", samples);
	for (i = 0; i < copies; i++) {
		st[i] = qd_job_exit_status(ids[i]);
		printf("PGRUN: exit %d status %x\n", i + 1, st[i]);
	}
	for (i = 0; i < copies; i++)
		while (!qd_job_reclaimed(ids[i]))
			qd_yield(1);
	if (qd_pginfo(0, &after) != 0 || after.spaces_live != base.spaces_live ||
	    after.held_bytes != base.held_bytes) {
		printf("PGRUN: released no\n");
		ok = 0;
	} else {
		printf("PGRUN: released yes\n");
	}
	if (ok) {
		printf("PGRUN: DONE\n");
		return 0;
	}
	printf("PGRUN: FAILED\n");
	return 5;
}
