/*
 * qdos.c - QDOS/386 R8 SDK wrappers over the existing traps (libqdos.a).
 * Freestanding C89.  Each wrapper is a thin call to qtrap; no buffering,
 * no locale, no stdio.
 */
#include "qdos386.h"

/* Unwinds to the job's entry frame (qtrap.s), so the job ends without a return
 * from main.  Returns only if crt0 did not record an entry frame. */
void qd_end_job(void);

void qd_exit(int status)
{
	qtrap(QD_TRAP_EXIT, (unsigned int) status, 0, 0);
	qd_end_job();
	for (;;)
		qtrap(QD_TRAP_SUSPEND, (unsigned int) -1, 0, 0);
}

/* Records the exit status and returns.  The kernel ends the job when its entry
 * point returns, so main's return path (crt0_libc.s) uses this, not qd_exit. */
void qd_exit_record(int status)
{
	qtrap(QD_TRAP_EXIT, (unsigned int) status, 0, 0);
}

int qd_args(char ***argv)
{
	unsigned int block = 0;
	int argc;

	argc = (int) qtrap(QD_TRAP_APP_ARGS, (unsigned int) &block, 0, 0);
	if (argv != 0)
		*argv = (char **) block;
	return argc;
}

unsigned long qd_ticks(void)
{
	return (unsigned long) qtrap(QD_TRAP_TICKS, 0, 0, 0);
}

/* R11F: the kernel wall clock (see docs/R11F-TIME-ABI.md).  0xFFFFFFFF means
 * unavailable: no clock is configured in this kernel build, or it overflowed. */
unsigned long qd_wall_time(void)
{
	return (unsigned long) qtrap(QD_TRAP_WALL, 0, 0, 0);
}

void qd_yield(unsigned int ticks)
{
	qtrap(QD_TRAP_SUSPEND, ticks, 0, 0);
}

void *qd_alloc(unsigned int bytes)
{
	int p = (int) qtrap(QD_TRAP_MEM_ALLOC, bytes, 0, 0);

	if (p <= 0)
		return 0;
	return (void *) p;
}

int qd_free(void *block)
{
	return (int) qtrap(QD_TRAP_MEM_FREE, (unsigned int) block, 0, 0);
}

unsigned int qd_free_bytes(void)
{
	return (unsigned int) qtrap(QD_TRAP_MEM_FREE_BYTES, 0, 0, 0);
}

int qd_heap_ok(void)
{
	return qtrap(QD_TRAP_MEM_VALIDATE, 0, 0, 0) == 0;
}

int qd_open(const char *name)
{
	return (int) qtrap(QD_TRAP_CH_OPEN, (unsigned int) name, 0, 0);
}

int qd_close(int ch)
{
	return (int) qtrap(QD_TRAP_CH_CLOSE, (unsigned int) ch, 0, 0);
}

int qd_write(int ch, const void *buf, unsigned int n)
{
	return (int) qtrap(QD_TRAP_CH_WRITE, (unsigned int) ch, (unsigned int) buf, n);
}

int qd_putc(int ch, int c)
{
	return (int) qtrap(QD_TRAP_CH_PUTC, (unsigned int) ch, (unsigned int) c, 0);
}

int qd_read(int ch, void *buf, unsigned int n)
{
	int got;

	if (n > QD_READ_MAX)
		n = QD_READ_MAX;
	got = (int) qtrap(QD_TRAP_CH_READ, (unsigned int) ch, (unsigned int) buf, n);
	return got;
}

int qd_seek(int ch, unsigned long pos)
{
	return (int) qtrap(QD_TRAP_CH_SEEK, (unsigned int) ch, (unsigned int) pos, 0);
}

int qd_puts(const char *s)
{
	unsigned int n = 0;
	int ch = qd_open("CON");

	if (ch <= 0)
		return ch;
	while (s[n] != 0)
		n++;
	n = (unsigned int) qd_write(ch, s, n);
	qd_close(ch);
	return (int) n;
}

/* ---- R11E: jobs, heap, console and directory wrappers --------------------- */

int qd_self(void)
{
	return (int) qtrap(QD_TRAP_JOB_SELF, 0, 0, 0);
}

int qd_job_slot(unsigned int slot)
{
	return (int) qtrap(QD_TRAP_JOB_SLOT, slot, 0, 0);
}

int qd_job_info(int id, unsigned int *inc, unsigned int *acc)
{
	int r = (int) qtrap(QD_TRAP_JOB_INFO, (unsigned int) id, 0, 0);

	if (r < 0)
		return r;
	if (inc != 0)
		*inc = (unsigned int) r & 0xffu;
	if (acc != 0)
		*acc = ((unsigned int) r >> 8) & 0xffu;
	return 0;
}

int qd_job_activate(int id, unsigned int inc)
{
	return (int) qtrap(QD_TRAP_JOB_ACTIVATE, (unsigned int) id, inc & 0xffu, 0);
}

int qd_job_chprio(int id, unsigned int inc)
{
	return (int) qtrap(QD_TRAP_JOB_CHPRIO, (unsigned int) id, inc & 0xffu, 0);
}

int qd_job_reactivate(int id)
{
	return (int) qtrap(QD_TRAP_JOB_REACT, (unsigned int) id, 0, 0);
}

/* The kernel sets the job's timeout and sleeps; the hold pointer is passed as
 * 0 (no release flag), so no caller memory is written by the kernel. */
int qd_job_suspend(long ticks)
{
	return (int) qtrap(QD_TRAP_SUSPEND, (unsigned int) ticks, 0, 0);
}

int qd_job_exit_status(int id)
{
	return (int) qtrap(QD_TRAP_JOB_EXIT, (unsigned int) id, 0, 0);
}

int qd_job_reclaimed(int id)
{
	return (int) qtrap(QD_TRAP_JOB_RECLAIMED, (unsigned int) id, 0, 0);
}

/* Loads name (an 8.3 file name on WIN1_) as a new job in child slot 1.  args,
 * if not NULL, is one NUL-terminated string that the kernel passes as the
 * argument block; its length including the NUL is what the kernel gets. */
int qd_run(const char *name, const char *args)
{
	unsigned int n = 0;

	if (args == 0)
		return (int) qtrap(QD_TRAP_IMG_LOAD, (unsigned int) name, 0, 0);
	while (args[n] != '\0')
		n++;
	return (int) qtrap(QD_TRAP_IMG_LOAD, (unsigned int) name,
	                   (unsigned int) args, n + 1);
}

/* R11H: start a named WIN1_ root file at a profile, or any (-1).  No arguments. */
int qd_start(const char *name, int profile)
{
	return (int) qtrap(QD_TRAP_IMG_START, (unsigned int) name, (unsigned int) profile, 0);
}

int qd_con_wait(unsigned char *flag)
{
	return (int) qtrap(QD_TRAP_CON_WAIT, 0, (unsigned int) flag, 0);
}

int qd_alloc_ex(unsigned int bytes, void **out)
{
	int p = (int) qtrap(QD_TRAP_MEM_ALLOC, bytes, 0, 0);

	if (out != 0)
		*out = 0;
	if (p < 0)
		return p;
	if (p == 0)
		return -3;                    /* MEM_ERR_NOMEM: the kernel never returns 0 */
	if (out != 0)
		*out = (void *) p;
	return 0;
}

int qd_mem_validate(void)
{
	return (int) qtrap(QD_TRAP_MEM_VALIDATE, 0, 0, 0);
}

int qd_getc(int ch)
{
	unsigned char c = 0;
	int n = qd_read(ch, &c, 1);

	if (n == 1)
		return (int) c;
	if (n == 0)
		return QD_EOF;
	return n < 0 ? n : -1;
}

int qd_dir_open(void)
{
	return qd_open("WIN1_");
}

int qd_dir_next(int ch, struct qd_dirent *e)
{
	unsigned char rec[16];
	int n = qd_read(ch, rec, 16);
	unsigned int i;

	if (n == 0)
		return 0;
	if (n < 0)
		return n;
	if (n != 16)
		return -1;
	for (i = 0; i < 12; i++)
		e->name[i] = (char) rec[i];
	e->name[12] = '\0';
	e->size = (unsigned long) rec[12] | ((unsigned long) rec[13] << 8)
	        | ((unsigned long) rec[14] << 16) | ((unsigned long) rec[15] << 24);
	return 1;
}
