/* FAULTT.AOU (R12A): a process's own faults and bad pointers.
 *   usage: FAULTT mode     mode = null | wild | ud | div | unmapped | ptr
 * null      write to address 0 (unmapped)          -> #PF, the process is ended
 * wild      jump to an unmapped address             -> #PF on the instruction fetch, the process is ended
 * ud        an invalid opcode (0F 0B)                -> #UD, the process is ended
 * div       integer divide by zero                   -> #DE, the process is ended
 * unmapped  read a window page past the image        -> #PF, the process is ended
 * ptr       hand bad pointers to the kernel: every call must be refused (CHAN_ERR_RANGE or another error), the process
 *           keeps running and prints FAULTT ptr PASS.
 * hold N    N times: suspend for 2 ticks with a hold flag in this process's BSS, between two sentinel bytes; the kernel
 *           sets the flag when the timeout expires - usually while another process's directory is active, so the write
 *           must be deferred to this process's own directory.  Checks the flag and both sentinels every time.
 * The kernel report names the vector, error code, CR2, EIP, job id and CR3. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "qdos386.h"

static volatile int zero = 0;
static volatile unsigned char hold[16];
static int checks, fails;

static void expect_refused(const char *what, int rc)
{
	checks++;
	if (rc >= 0) {
		fails++;
		printf("FAULTT ptr FAIL %s accepted (%d)\n", what, rc);
	}
}

int main(int argc, char **argv)
{
	const char *m = argc > 1 ? argv[1] : "ptr";
	int con;

	printf("FAULTT %s start\n", m);
	if (strcmp(m, "null") == 0) {
		*(volatile int *) 0 = 1;
	} else if (strcmp(m, "wild") == 0) {
		void (* volatile f)(void);   /* through a variable: GCC 1.40 turns a call to a constant into a pc-relative call */
		f = (void (*)(void)) 0x00330000u;
		(*f)();
	} else if (strcmp(m, "ud") == 0) {
		asm(".byte 0x0f, 0x0b");
	} else if (strcmp(m, "div") == 0) {
		printf("FAULTT div %d\n", 10 / zero);
	} else if (strcmp(m, "unmapped") == 0) {
		printf("FAULTT unmapped %d\n", *(volatile int *) 0x00330000u);
	} else if (strcmp(m, "hold") == 0) {
		int n = argc > 2 ? atoi(argv[2]) : 100, i, bad = 0;
		struct qd_pginfo pi0, pi1;
		qd_pginfo(0, &pi0);
		for (i = 0; i < n; i++) {
			hold[3] = 0x5A;
			hold[5] = 0xA5;
			hold[4] = 0;
			qtrap(0x08u, 2u, (unsigned int) &hold[4], 0);
			if (hold[4] != 0xFF || hold[3] != 0x5A || hold[5] != 0xA5)
				bad++;
		}
		qd_pginfo(0, &pi1);
		printf("FAULTT hold %s n=%d bad=%d\n", bad == 0 ? "PASS" : "FAIL", n, bad);
		return bad == 0 ? 0 : 3;
	} else if (strcmp(m, "ptr") == 0) {
		con = qd_open("CON");
		expect_refused("write from NULL", qd_write(con, (void *) 0, 8));
		expect_refused("write from kernel text 0x100000", qd_write(con, (void *) 0x00100000u, 8));
		expect_refused("write from the kernel heap 0x180000", qd_write(con, (void *) 0x00180000u, 8));
		expect_refused("write from an unmapped window page 0x330000", qd_write(con, (void *) 0x00330000u, 8));
		expect_refused("write crossing the stack top", qd_write(con, (void *) 0x1FFFFFF8u, 16));
		expect_refused("write past the heap end", qd_write(con, (void *) 0x10F00000u, 16));
		expect_refused("wrapping range", qd_write(con, (void *) 0xFFFFFFF0u, 0x20));
		expect_refused("read into the transfer area", qd_read(con, (void *) 0x00340000u, 1));
		expect_refused("open with a NULL name", qd_open((const char *) 0));
		expect_refused("open with a name in the boot stack", qd_open((const char *) 0x007FFF00u));
		expect_refused("suspend with a hold flag in kernel memory",
		               (int) qtrap(0x08u, 1u, 0x00101000u, 0));
		expect_refused("proc info into kernel memory", qd_proc_info(0, (struct qd_procinfo *) 0x00100000u));
		printf("FAULTT ptr %s checks=%d fails=%d\n", fails == 0 ? "PASS" : "FAIL", checks, fails);
		return fails == 0 ? 0 : 3;
	}
	printf("FAULTT %s FAIL still running\n", m);
	return 3;
}
