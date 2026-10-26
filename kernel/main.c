/*
 * QDOS/386 R1 - kernel entry (SYSTEM job), idle job and final validation.
 *
 * Job 0 SYSTEM  = boot context (this function).  Suspends itself once the
 *                 application jobs are created.
 * Job 1 TEST-A  = CPU-bound test job.
 * Job 2 TEST-B  = CPU-bound test job.
 * Job 3 IDLE    = lowest-priority job.  When no application job remains it
 *                 validates the run and prints the PASS/FAIL line.
 */
#include "jobs.h"
#include "platform.h"
#include "test_jobs.h"

#define TIMER_PERIOD_UNITS 16          /* IRQ0 every 16 * 1024 = 16384 steps */
#define JOB_SLOT_TEST_A 1
#define JOB_SLOT_TEST_B 2
#define JOB_SLOT_IDLE 3

static unsigned int stack_pool[JOB_MAX][JOB_STACK_WORDS];
static int report_done = 0;

static void finish(void)
{
	int ok = 1;
	int i;
	unsigned int device_ticks;

	con_puts("IDLE: no application jobs remain\n");

	for (i = 0; i < 2; i++) {
		struct job *j = &jobs[JOB_SLOT_TEST_A + i];
		con_puts(i == 0 ? "TEST-A" : "TEST-B");
		con_puts(": preempted ");
		con_putdec(j->preempts);
		con_puts(" times\n");
		if (j->status != JOB_DEAD)
			ok = 0;
		if (test_results[i].iterations != TEST_ITERATIONS)
			ok = 0;
		if (test_results[i].canary_errors != 0)
			ok = 0;
		if (i == 0 && !test_results[i].coop_resumed)
			ok = 0;
		if (j->preempts == 0)
			ok = 0;
	}

	/* The device count must match the scheduler's count (low 8 bits). */
	device_ticks = timer_delivered();
	if (device_ticks != (unsigned int) (sched_ticks & 0xff))
		ok = 0;
	timer_stop();

	con_puts("TIMER: ticks ");
	con_putdec(sched_ticks);
	con_puts("\nCOOPERATIVE YIELDS: ");
	con_putdec(sched_yields);
	con_puts("\nIDLE DISPATCHES: ");
	con_putdec(jobs[JOB_SLOT_IDLE].dispatches);
	con_putc('\n');

	if (ok) {
		con_puts("\nQDOS/386 R1 MULTITASKING PASS\n");
		halt_with(0);
	}
	con_puts("\nQDOS/386 R1 MULTITASKING FAIL\n");
	halt_with(1);
}

static void idle_entry(void)
{
	for (;;) {
		if (!report_done &&
		    jobs[JOB_SLOT_TEST_A].status == JOB_DEAD &&
		    jobs[JOB_SLOT_TEST_B].status == JOB_DEAD) {
			report_done = 1;
			finish();
		}
	}
}

int main(void)
{
	unsigned int cr0 = read_cr0();

	con_puts("QDOS/386 R1\n");
	con_puts("i386VM native boot\n\n");

	con_puts("CPU: i386 protected mode\n");
	if (cr0 & 1u)
		con_puts("Protected mode: OK\n");
	else
		con_puts("Protected mode: FAIL\n");
	if (cr0 & 0x80000000u)
		con_puts("Paging: ENABLED (unexpected)\n");
	else
		con_puts("Paging: disabled\n");

	descriptors_init();
	con_puts("Kernel: initialized\n\n");

	sched_init();
	job_register_boot("SYSTEM", 1);
	con_puts("SYSTEM: job 0 created\n");

	job_create("TEST-A", test_job_a, 4,
	           (unsigned int) stack_pool[JOB_SLOT_TEST_A], sizeof(stack_pool[0]));
	con_puts("TEST-A: job 1 created\n");
	job_create("TEST-B", test_job_b, 4,
	           (unsigned int) stack_pool[JOB_SLOT_TEST_B], sizeof(stack_pool[0]));
	con_puts("TEST-B: job 2 created\n");
	job_create("IDLE", idle_entry, 1,
	           (unsigned int) stack_pool[JOB_SLOT_IDLE], sizeof(stack_pool[0]));

	con_puts("\nTIMER: IRQ0 active\n");
	con_puts("SCHEDULER: preemption enabled\n\n");
	timer_start(TIMER_PERIOD_UNITS);

	job_suspend_self();     /* SYSTEM is not resumed; finish() ends the run */
	for (;;)
		;
	return 0;
}
