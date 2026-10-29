/* QDOS/386 R12A - libqdos process entry points (kernels built with -DQDOS_DYNPROC).  A separate archive member, so
 * programs that do not call them link exactly the bytes they linked before.  On an older kernel every call answers
 * QD_PROC_ERR_NONE (the trap numbers are unknown there and return -1).  See docs/R12A-PROCESS-ABI.md. */
#include "qdos386.h"

#define QD_TRAP_PROC_CAPS   0x2Fu
#define QD_TRAP_PROC_SPAWN  0x30u
#define QD_TRAP_PROC_WAIT   0x31u
#define QD_TRAP_PROC_INFO   0x32u
#define QD_TRAP_PROC_KILL   0x33u
#define QD_TRAP_PROC_VA     0x34u

int qd_proc_caps(void)
{
	int c = (int) qtrap(QD_TRAP_PROC_CAPS, 0, 0, 0);

	return c > 0 ? c : QD_PROC_ERR_NONE;
}

int qd_proc_info(int id, struct qd_procinfo *out)
{
	int rc;

	if (qd_proc_caps() < 0)
		return QD_PROC_ERR_NONE;
	rc = (int) qtrap(QD_TRAP_PROC_INFO, (unsigned int) id, (unsigned int) out, 0);
	return rc;
}

unsigned long qd_proc_va(const void *p)
{
	return (unsigned long) (unsigned int) qtrap(QD_TRAP_PROC_VA, (unsigned int) p, 0, 0);
}

int qd_spawn(const char *name, const char *packed, unsigned int len, unsigned int flags, int console)
{
	unsigned int req[5];

	req[0] = 1u;                         /* request version */
	req[1] = (unsigned int) packed;
	req[2] = len;
	req[3] = flags;
	req[4] = (unsigned int) console;
	if (qd_proc_caps() < 0)
		return QD_PROC_ERR_NONE;
	return (int) qtrap(QD_TRAP_PROC_SPAWN, (unsigned int) name, (unsigned int) req, 0);
}

#define QD_PROC_AGAIN (-73)          /* the kernel woke a blocked wait: ask again */

int qd_wait(int id, long timeout_ticks)
{
	unsigned long t0 = qd_ticks(), el;
	long rem;
	int rc;

	for (;;) {
		rem = timeout_ticks;
		if (timeout_ticks > 0) {
			el = qd_ticks() - t0;
			rem = (long) el >= timeout_ticks ? 0 : timeout_ticks - (long) el;
		}
		rc = (int) qtrap(QD_TRAP_PROC_WAIT, (unsigned int) id, (unsigned int) rem, 0);
		if (rc != QD_PROC_AGAIN)
			return rc;
	}
}

int qd_poll(int id)
{
	return (int) qtrap(QD_TRAP_PROC_WAIT, (unsigned int) id, 0, 0);
}

int qd_kill(int id)
{
	return (int) qtrap(QD_TRAP_PROC_KILL, (unsigned int) id, 0, 0);
}
