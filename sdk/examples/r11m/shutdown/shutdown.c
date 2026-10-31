/* SHUTDOWN: R11M-FIX1 guest helper.  Asks the emulator to end the machine cleanly, so that it can flush and close its
 * disk images, by writing the 8-byte string "Shutdown" to I/O port 0x8900 (the Bochs/QEMU-compatible shutdown port that
 * Tiny386 decodes; see docs/R11M-FIX1-SHUTDOWN-ABI.md).  It never powers a real machine off and it does nothing on
 * emulators that do not decode the port.
 *
 *   SHUTDOWN          check, then request the shutdown
 *   SHUTDOWN /N       check only ("SHUTDOWN: ready" or the refusal); nothing is written to the port
 *   SHUTDOWN /W       test only: writes the string "Shutdowm" (wrong last letter); a correct emulator ignores it and keeps
 *                     running, so the program reaches "SHUTDOWN: no response" (status 2)
 *
 * Before the request it
 *   - refuses (exit status 1) if any other launched job is still in the job table (a job could be in the middle of a
 *     file system operation; the kernel's file system is write-through, so a finished operation is already on the disk),
 *   - flushes the C library streams.
 * After the request it waits a bounded time (about two seconds).  The emulator normally ends the machine before the wait
 * is over.  If it is still running, the request was not acted on: it prints "SHUTDOWN: no response" and exits with
 * status 2 (that is the normal result on an emulator without the port, e.g. the CPU-only i386VM harness).
 * Exit status 0 is never seen after a successful request: the machine is gone.  All code runs in ring 0, so the
 * program issues the port writes itself. */
#include <stdio.h>
#include <string.h>
#include "qdos386.h"

#define SHUTDOWN_PORT 0x8900

static void out8(unsigned int port, unsigned int value)
{
	asm volatile ("outb %%al,%%dx" : : "a" (value), "d" (port));
}

int main(int argc, char **argv)
{
	struct qd_jobinfo all[8];
	int n, i, others = 0, check_only = 0;
	char magic[9];

	strcpy(magic, "Shutdown");

	if (argc > 1 && (strcmp(argv[1], "/N") == 0 || strcmp(argv[1], "/n") == 0))
		check_only = 1;
	if (argc > 1 && (strcmp(argv[1], "/W") == 0 || strcmp(argv[1], "/w") == 0))
		magic[7] = 'm';
	n = qd_job_list(all, 8);
	for (i = 0; i < n; i++) {
		if ((all[i].flags & QD_JF_LAUNCHED) && !(all[i].flags & QD_JF_SELF) && all[i].state != QD_JS_DEAD)
			others++;
	}
	if (others > 0) {
		printf("SHUTDOWN: refused, %d other job(s) still running (use PS and KILL)\n", others);
		fflush(stdout);
		return 1;
	}
	if (check_only) {
		printf("SHUTDOWN: ready\n");
		fflush(stdout);
		return 0;
	}
	printf("SHUTDOWN: requesting\n");
	fflush(stdout);
	fflush(NULL);
	for (i = 0; i < 8; i++)
		out8(SHUTDOWN_PORT, (unsigned char) magic[i]);
	for (i = 0; i < 200; i++)
		qd_yield(1);
	printf("SHUTDOWN: no response\n");
	fflush(stdout);
	return 2;
}
