/*
 * QDOS/386 R3 - SYSTEM job, idle job and final validation.
 *
 * Boot.  Initialise the descriptors and scheduler, then the kernel heap, then
 *        run the heap-only tests (no jobs, no timer).
 * Job 0 SYSTEM  boot context.  Creates TEST-A and TEST-B by service trap,
 *               runs the R2 scheduler checks, then the R3 job phase
 *               (memory.c used by preempted jobs, reclaim, slot reuse,
 *               creation under pressure), then suspends indefinitely.
 * Job 3 IDLE    lowest priority.  When every application job has exited and
 *               the R3 phase is done, it prints every check and the result.
 */
#include "jobs.h"
#include "memory.h"
#include "platform.h"
#include "test_jobs.h"
#include "test_memory.h"

#define TIMER_PERIOD_UNITS 16        /* i386VM: 16 * 1024 steps.  PC: see platform */

/* Start of BSS, from the linker (C name bss_end -> symbol _bss_end). */
extern char bss_end[];

int test_system_id = 0;

static unsigned char flag_timeout = 0;
static unsigned char flag_rel = 0;
static int id_a = -1, id_b = -1, id_c = -1, id_idle = -1;
static int chk_chprio = 0, chk_timeout = 0, chk_release = 0;
static int chk_stale = 0, chk_newid = 0, chk_reuse = 0, chk_tasks = 0;
static unsigned long poll_at_start = 0, sched_at_start = 0;

struct qtask poll_node;
struct qtask sched_node;

static void poll_task(void)  { /* counted in poll_runs */ }
static void sched_task(void) { /* counted in sched_runs */ }

static int ok_word(int b) { return b ? 1 : 0; }

static int apps_done(void)
{
	return test_results[0].exited && test_results[1].exited &&
	       test_results[2].exited && test_results[0].coop_resumed == 1 &&
	       r3_phase_done;
}

static void finish(void)
{
	int ok = 1;
	int i;
	int dev_ok = 1;
	int r3_ok = 1;
	struct mem_stats st;

	con_puts("IDLE: no application jobs remain\n");

	for (i = 0; i < 2; i++) {
		con_puts(i == 0 ? "TEST-A" : "TEST-B");
		con_puts(": preempted ");
		con_puts(test_results[i].preempts > 0 ? "yes" : "no");
		con_putc('\n');
		if (test_results[i].iterations != TEST_ITERATIONS ||
		    test_results[i].canary_errors != 0 ||
		    test_results[i].preempts == 0)
			ok = 0;
	}
	if (!test_results[0].coop_resumed || !test_results[2].exited)
		ok = 0;

	if (timer_device_present()) {
		unsigned int dc = timer_device_count();
		dev_ok = (dc == (unsigned int) (sched_ticks & 0xff));
	}
	timer_stop();

	con_puts("CHECK chprio: ");        con_puts(ok_word(chk_chprio) ? "ok\n" : "FAIL\n");
	con_puts("CHECK timeout wake: ");  con_puts(ok_word(chk_timeout) ? "ok\n" : "FAIL\n");
	con_puts("CHECK reactivate: ");    con_puts(ok_word(chk_release) ? "ok\n" : "FAIL\n");
	con_puts("CHECK poll/sched lists: "); con_puts(ok_word(chk_tasks) ? "ok\n" : "FAIL\n");
	con_puts("CHECK slot reuse: ");    con_puts(ok_word(chk_reuse) ? "ok\n" : "FAIL\n");
	con_puts("CHECK stale id: ");      con_puts(ok_word(chk_stale) ? "ok\n" : "FAIL\n");
	con_puts("CHECK new id: ");        con_puts(ok_word(chk_newid) ? "ok\n" : "FAIL\n");
	if (timer_device_present()) {
		con_puts("CHECK device ticks: "); con_puts(dev_ok ? "ok\n" : "FAIL\n");
	} else {
		con_puts("CHECK device ticks: n/a\n");
	}

	if (!chk_chprio || !chk_timeout || !chk_release || !chk_tasks ||
	    !chk_reuse || !chk_stale || !chk_newid || !dev_ok)
		ok = 0;

	for (i = 0; i < r3_nchecks; i++) {
		con_puts("CHECK ");
		con_puts(r3_checks[i].name);
		con_puts(r3_checks[i].ok ? ": ok\n" : ": FAIL\n");
		if (!r3_checks[i].ok)
			r3_ok = 0;
	}
	if (r3_nchecks == 0)
		r3_ok = 0;

	mem_stats(&st);
	con_puts("HEAP: total ");
	con_putdec(st.total);
	con_puts(" used ");
	con_putdec(st.used);
	con_puts(" free ");
	con_putdec(st.free);
	con_puts(" blocks ");
	con_putdec(st.used_blocks);
	con_puts("/");
	con_putdec(st.free_blocks);
	con_putc('\n');
	con_puts("RECLAIM: jobs ");
	con_putdec(jobs_reclaimed);
	con_puts(" bytes ");
	con_putdec(bytes_reclaimed);
	con_putc('\n');
	con_puts("STACK: faults ");
	con_putdec(stack_faults);
	con_puts(" hwm MEM-1 ");
	con_putdec(test_results[M1_SLOT_RESULT_IDX].hwm);
	con_putc('\n');

	con_puts("TIMER: ticks ");
	con_putdec(sched_ticks);
	con_puts("\nCOOPERATIVE YIELDS: ");
	con_putdec(sched_yields);
	con_puts("\nSCHEDULER PASSES: ");
	con_putdec(sched_passes);
	con_puts("\nPOLL RUNS: ");
	con_putdec(poll_runs);
	con_puts("\nSCHEDULE RUNS: ");
	con_putdec(sched_runs);
	con_puts("\nIDLE DISPATCHES: ");
	con_putdec(jobs[job_idle].dispatches);
	con_putc('\n');

	if (ok && r3_ok && mem_validate() == MEM_OK && stack_faults == 0) {
		con_puts("\nQDOS/386 R3 MEMORY PASS\n");
		halt_with(0);
	}
	con_puts("\nQDOS/386 R3 MEMORY FAIL\n");
	halt_with(1);
}

static void idle_entry(void)
{
	static int report_done = 0;
	for (;;) {
		if (!report_done && apps_done()) {
			report_done = 1;
			finish();
		}
	}
}

int main(void)
{
	unsigned int cr0 = read_cr0();
	unsigned long t0;
	unsigned int arena;
	int i;

	con_puts("QDOS/386 R3\n");
	con_puts("Native i386 kernel\n\n");
	con_puts("CPU: i386 protected mode\n");
	con_puts((cr0 & 1u) ? "Protected mode: OK\n" : "Protected mode: FAIL\n");
	con_puts((cr0 & 0x80000000u) ? "Paging: ENABLED (unexpected)\n"
	                             : "Paging: disabled\n");

	descriptors_init();
	sched_init();

	arena = ((unsigned int) bss_end + MEM_ALIGN - 1) & ~(MEM_ALIGN - 1);
	if (mem_init(arena, MEM_ARENA_END) != MEM_OK) {
		con_puts("Heap: arena rejected\n");
		halt_with(1);
	}
	con_puts("Heap: arena ");
	con_putdec(arena);
	con_puts(" to ");
	con_putdec(MEM_ARENA_END);
	con_putc('\n');
	memory_heap_tests();
	con_puts("Heap: heap-only tests done\n");

	con_puts("Kernel: initialized\n\n");
	con_puts("SYSTEM: job 0 created\n");

	id_a = qtrap(QT_CREATE, (unsigned int) test_job_a, 4, (unsigned int) "TEST-A");
	con_puts("TEST-A: job created by trap\n");
	id_b = qtrap(QT_CREATE, (unsigned int) test_job_b, 4, (unsigned int) "TEST-B");
	con_puts("TEST-B: job created by trap\n");
	id_idle = job_alloc("IDLE", idle_entry, 1);
	job_idle_set(id_idle & 0xffff);
	test_system_id = 0;

	/* Priority change: TEST-B goes from 4 to 6, read back with INFO. */
	chk_chprio = (qtrap(QT_CHPRIO, (unsigned int) id_b, 6, 0) == 0) &&
	             ((qtrap(QT_INFO, (unsigned int) id_b, 0, 0) & 0xff) == 6);

	poll_node.fn = poll_task;
	sched_node.fn = sched_task;
	qtrap(QT_POLL_ADD, (unsigned int) &poll_node, 0, 0);
	qtrap(QT_SCHED_ADD, (unsigned int) &sched_node, 0, 0);

	con_puts("SCHEDULER: preemption enabled\n");
#ifndef QDOS_NO_TIMER
	timer_start(TIMER_PERIOD_UNITS);
	con_puts("TIMER: IRQ0 active\n\n");
#else
	con_puts("TIMER: disabled (negative test build)\n\n");
#endif

	/* Timeout: suspend SYSTEM for 3 ticks with a release flag. */
	poll_at_start = poll_runs;
	sched_at_start = sched_runs;
	t0 = sched_ticks;
	flag_timeout = 0;
	qtrap(QT_SUSPEND, 3, (unsigned int) &flag_timeout, 0);
	chk_timeout = (sched_ticks - t0 >= 3) && (flag_timeout == 0xff);

	/* Indefinite suspend: TEST-B reactivates SYSTEM early. */
	flag_rel = 0;
	qtrap(QT_SUSPEND, -1, (unsigned int) &flag_rel, 0);
	chk_release = (flag_rel == 0xff);

	/* Wait for TEST-A to exit and its slot to be reclaimed. */
	while (!test_results[0].exited)
		qtrap(QT_SUSPEND, 1, 0, 0);
	while (jobs[id_a & 0xffff].in_use)
		qtrap(QT_SUSPEND, 1, 0, 0);

	/* Stale id: TEST-A's id must now fail. */
	chk_stale = (qtrap(QT_REACTIVATE, (unsigned int) id_a, 0, 0) == -2);

	/* Reuse the slot for TEST-C with a new tag. */
	id_c = qtrap(QT_CREATE, (unsigned int) test_job_c, 2, (unsigned int) "TEST-C");
	chk_reuse = (id_c >= 0) && ((id_c & 0xffff) == (id_a & 0xffff));
	chk_newid = (id_c >= 0) && (id_c != id_a) &&
	            (qtrap(QT_REACTIVATE, (unsigned int) id_c, 0, 0) == 0) &&
	            (qtrap(QT_ACTIVATE, (unsigned int) id_c, 3, 0) == -1);  /* already active */

	chk_tasks = (poll_runs > poll_at_start) && (sched_runs > sched_at_start);

	/* R3 job phase: preempted allocators, reclaim, slot reuse, pressure. */
	memory_job_phase();

	/* Nothing left for SYSTEM: suspend indefinitely.  IDLE finishes the run. */
	for (i = 0; ; i++) {
		qtrap(QT_SUSPEND, -1, 0, 0);
	}
	return 0;
}
