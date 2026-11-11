/* QDOS/386 R11K - libqdos paging entry points (opt-in kernels built with -DQDOS_PAGING).
 *
 * Kept in a separate archive member (qdospg.o) so a program that does not call them links exactly the
 * bytes it linked before R11K.  On a kernel without paging every call returns QD_PG_ERR_NOPAGING
 * (or 0 from qd_pg_available) - the trap numbers are unknown there and answer -1.
 * See docs/R11K-PAGING-APP-ABI.md.  All limits, including that the heap and the stacks are shared by
 * every process, are listed there.  No physical frame address is exposed. */
#include "qdos386.h"

#define QD_TRAP_PG_START   0x25u
#define QD_TRAP_PG_INFO    0x26u
#define QD_TRAP_PG_CAPS    0x27u
#define QD_TRAP_PG_CANCEL  0x29u

#define QD_PG_FLAG_HELD    0x00010000u

/* A kernel without paging answers every unknown trap with -1, which no paging call returns itself. */
static int nopaging(int rc)
{
	return rc == -1 ? QD_PG_ERR_NOPAGING : rc;
}

int qd_pg_caps(void)
{
	int c = (int) qtrap(QD_TRAP_PG_CAPS, 0, 0, 0);

	/* A non-paging kernel answers -1 (unknown trap).  A real answer is positive: the ABI version is
	 * in bits 24..30. */
	return c > 0 ? c : QD_PG_ERR_NOPAGING;
}

int qd_pg_available(void)
{
	int c = qd_pg_caps();

	return c > 0 && (c & QD_PG_CAP_START) != 0;
}

int qd_pg_startv(const char *name, const char *packed, unsigned int len, unsigned int flags)
{
	unsigned int c = len;

	if ((flags & ~QD_PG_HELD) != 0 || len > 0xFFFFu)
		return QD_PG_ERR_BADFLAGS;
	if (flags & QD_PG_HELD)
		c |= QD_PG_FLAG_HELD;
	return nopaging((int) qtrap(QD_TRAP_PG_START, (unsigned int) name, (unsigned int) packed, c));
}

int qd_pg_start(const char *name, const char *args, unsigned int flags)
{
	unsigned int n = 0;

	if (args != 0) {
		while (args[n] != '\0')
			n++;
		return qd_pg_startv(name, args, n + 1, flags);
	}
	return qd_pg_startv(name, 0, 0, flags);
}

int qd_pg_cancel(int id)
{
	return nopaging((int) qtrap(QD_TRAP_PG_CANCEL, (unsigned int) id, 0, 0));
}

int qd_pginfo(int id, struct qd_pginfo *out)
{
	unsigned int w[18];
	int rc = (int) qtrap(QD_TRAP_PG_INFO, (unsigned int) id, (unsigned int) w, 0);

	if (rc != 0)
		return nopaging(rc);
	out->paged = w[0];
	out->space = w[1] >> 12;       /* opaque: equal for jobs that share an address space, nothing more */
	out->vbase = w[4];
	out->vsize = w[5];
	out->dispatches = w[9];
	out->preempts = w[8];
	out->spaces_live = w[15];
	out->held_bytes = w[6];
	return 0;
}
