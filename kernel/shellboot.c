/*
 * QDOS/386 R7 - shell boot.  See shellboot.h.  Compiled into every image, but
 * entered only from main() when R7_SHELL_MODE is defined.
 */
#include "shellboot.h"
#include "jobs.h"
#include "fpu.h"
#include "memory.h"
#include "platform.h"
#include "clock_boot.h"
#include "aout.h"
#include "imgload.h"
#include "win1.h"
#include "cin.h"
#include "irq.h"
#ifdef QDOS_PAGING
#include "paging.h"
#endif
#ifdef QDOS_TEST_BIGK
extern int bigk_check(void);   /* R11K TEST ONLY: platform/pc/bigk_check.c */
#endif

#define SHELL_TIMER_UNITS 16        /* same period as the R6 test build */

extern char bss_end[];
void descriptors_init(void);

static void boot_idle(void)
{
	for (;;)
		;
}

static void boot_fail(const char *why)
{
	con_puts("SHELL: ");
	con_puts(why);
	con_putc('\n');
	halt_with(SHELL_HALT_BOOTFAIL);
}

int shell_boot(void)
{
	unsigned int arena;
	int shell_id, idle_id, st;

	con_puts("QDOS/386 R7\n");
	con_puts("Native i386 kernel\n\n");
#ifdef QDOS_TEST_BIGK
	bigk_check();
#endif

	descriptors_init();
	input_init();                  /* i8042 and IRQ1 before the timer */
	sched_init();
	fpu_reset();                   /* R10: clean x87 before any job runs */
	if (fpu_selftest() != 0)
		boot_fail("x87 FPU not functional (enable [cpu] fpu = 1)");
	con_puts("FPU: x87 OK\n");

	arena = ((unsigned int) bss_end + MEM_ALIGN - 1) & ~(MEM_ALIGN - 1);
	if (mem_init(arena, MEM_ARENA_END) != MEM_OK)
		boot_fail("heap arena rejected");
#ifdef QDOS_PAGING
	pg_init();                     /* R11J: identity paging before the volume and shell */
#ifdef QDOS_TEST_PG_TASKS
	pg_test_tasks_register();      /* R11K TEST ONLY: kernel poll/schedule tasks */
#endif
#endif

	if (win1_mount() != FAT_OK)
		boot_fail("WIN1_ volume not mounted");
	con_puts("WIN1_: mounted\n");

	shell_id = img_load_kernel(SHELL_IMAGE_NAME, 0);
	if (shell_id <= 0) {
		con_puts("SHELL: load failed, error ");
		con_putdec((unsigned long) (-shell_id));
		con_putc('\n');
		halt_with(SHELL_HALT_BOOTFAIL);
	}
	con_puts("SHELL: loaded into slot 0\n");

	idle_id = job_alloc("IDLE", boot_idle, 1);
	if (idle_id < 0)
		boot_fail("no job for IDLE");
	job_idle_set(idle_id & 0xffff);

	cin_enable_poll();             /* i386VM: pull bytes from input_poll */
#ifndef QDOS_NO_TIMER
	clock_boot(1);
	timer_start(SHELL_TIMER_UNITS);
	con_puts("SHELL: running\n\n");
#else
	clock_boot(0);
	/* Negative build: no timer, so no tick polls input and the shell never
	 * reads a byte.  The run must stall; see docs/R7-RUNTIME-REPORT.md. */
	con_puts("SHELL: timer disabled (negative test build)\n\n");
#endif

	/* SYSTEM sleeps between checks; the shell's own job runs in between. */
	for (;;) {
		st = qtrap(QT_JOB_EXIT, (unsigned int) shell_id, 0, 0);
		if (st != JOB_ERR_RUNNING)
			break;
		qtrap(QT_SUSPEND, 1, 0, 0);
	}
	if (st == JOB_ERR_NOSTATUS) {
		con_puts("SHELL: ended without QT_EXIT\n");
		halt_with(SHELL_HALT_NOSTATUS);
	}
	if (st < 0) {
		con_puts("SHELL: exit status unavailable\n");
		halt_with(SHELL_HALT_NOSTATUS);
	}
	con_puts("\nSHELL: exited with status ");
	con_putdec((unsigned long) st);
	con_putc('\n');
	halt_with(st);
	return st;
}
