/*
 * qdosjob.c - R11M: SDK wrappers for job control (kernels built with -DQDOS_JOBCTL).  A separate libqdos.a member.
 * On a kernel without job control the traps answer -1, which these calls report as QD_ERR_NOJOBCTL.  Freestanding C89.
 * See docs/R11M-JOB-CONTROL-ABI.md.
 */
#include "qdos386.h"

#define QT_JOB_QUERY 0x2Du
#define QT_JOB_KILL  0x2Eu
#define HDR 16u
#define ENT 40u
#define MAXJ 8                    /* JOB_MAX is 6; room for a larger table */

static unsigned long rd32(const unsigned char *p)
{
	return (unsigned long) p[0] | ((unsigned long) p[1] << 8) | ((unsigned long) p[2] << 16) | ((unsigned long) p[3] << 24);
}

int qd_job_list(struct qd_jobinfo *out, int max)
{
	unsigned char buf[HDR + ENT * MAXJ];
	int n, i, k;

	if (out == 0 || max < 1)
		return QD_ERR_RANGE;
	if (max > MAXJ)
		max = MAXJ;
	n = (int) qtrap(QT_JOB_QUERY, (unsigned int) buf, HDR + ENT * (unsigned int) max, 1u);
	if (n == -1)
		return QD_ERR_NOJOBCTL;
	if (n < 0)
		return n;
	for (i = 0; i < n && i < max; i++) {
		const unsigned char *e = buf + HDR + (unsigned int) i * ENT;

		out[i].id = rd32(e);
		out[i].slot = e[4];
		out[i].state = e[5];
		out[i].prio = e[6];
		out[i].console = e[7];
		out[i].flags = e[8];
		out[i].dispatches = rd32(e + 12);
		out[i].age_ticks = rd32(e + 16);
		out[i].stack_bytes = rd32(e + 20);
		for (k = 0; k < 15 && e[24 + k]; k++)
			out[i].name[k] = (char) e[24 + k];
		out[i].name[k] = 0;
	}
	return n;
}

int qd_job_find(unsigned long id, struct qd_jobinfo *out)
{
	struct qd_jobinfo all[MAXJ];
	int n = qd_job_list(all, MAXJ), i;

	if (n < 0)
		return n;
	for (i = 0; i < n; i++)
		if (all[i].id == id) {
			if (out)
				*out = all[i];
			return 1;
		}
	return 0;
}

int qd_job_kill(unsigned long id)
{
	int r = (int) qtrap(QT_JOB_KILL, (unsigned int) id, 0, 0);

	return r == -1 ? QD_ERR_NOJOBCTL : r;
}
