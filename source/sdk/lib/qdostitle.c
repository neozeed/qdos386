/* QDOS/386 R12C - libqdos: program titles, the v2 task list, the task selector, storage SYNC.  A separate member.
 * Kernels without these services answer -1 to every call.  See docs/R12C-PROGRAM-TITLE-ABI.md,
 * docs/R12C-TASK-MANAGER-ABI.md and docs/R12C-SYNC-DURABILITY.md. */
#include "qdos386.h"

#define QD_TRAP_JOB_TITLE 0x38u
#define QD_TRAP_SYNC      0x39u
#define QD_TRAP_TASKMGR   0x3Au
#define QD_TRAP_JOB_QUERY 0x2Du

int qd_set_title(const char *title)
{
	return (int) qtrap(QD_TRAP_JOB_TITLE, 0u, (unsigned int) title, 0u);
}

int qd_get_title(char *buf, unsigned int size)
{
	return (int) qtrap(QD_TRAP_JOB_TITLE, 2u, (unsigned int) buf, size);
}

int qd_exe_name(char *buf, unsigned int size)
{
	return (int) qtrap(QD_TRAP_JOB_TITLE, 3u, (unsigned int) buf, size);
}

/* Called by crt0 before main with the program's _prog_name (the SDK's empty default when the program defines none). */
void qd_prog_title(const char *name)
{
	if (name != 0 && name[0] != 0)
		(void) qtrap(QD_TRAP_JOB_TITLE, 1u, (unsigned int) name, 0u);
}

int qd_task_list(void *buf, unsigned int bytes)
{
	return (int) qtrap(QD_TRAP_JOB_QUERY, (unsigned int) buf, bytes, 2u);
}

int qd_taskmgr_open(void)
{
	return (int) qtrap(QD_TRAP_TASKMGR, 0u, 0u, 0u);
}

int qd_sync(void)
{
	return (int) qtrap(QD_TRAP_SYNC, 0u, 0u, 0u);
}

int qd_sync_status(unsigned long *words16)
{
	return (int) qtrap(QD_TRAP_SYNC, 1u, (unsigned int) words16, 0u);
}

int qd_sync_interval(unsigned int seconds)
{
	return (int) qtrap(QD_TRAP_SYNC, 2u, seconds, 0u);
}
