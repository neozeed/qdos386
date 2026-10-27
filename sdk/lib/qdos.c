/*
 * qdos.c - QDOS/386 R8 SDK wrappers over the existing traps (libqdos.a).
 * Freestanding C89.  Each wrapper is a thin call to qtrap; no buffering,
 * no locale, no stdio.
 */
#include "qdos386.h"

void qd_exit(int status)
{
	qtrap(QD_TRAP_EXIT, (unsigned int) status, 0, 0);
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
