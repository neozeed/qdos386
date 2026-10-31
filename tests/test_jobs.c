/*
 * QDOS/386 R1 - application test jobs.
 */
#include "platform.h"
#include "jobs.h"
#include "test_jobs.h"

struct test_result test_results[TEST_RESULTS];
unsigned int test_flag_rel = 0;
extern int test_system_id;              /* SYSTEM's job id, set by main */

static volatile unsigned int spin_sink;

void spin_ticks(unsigned long n)
{
	unsigned long start = sched_ticks;
	unsigned int s = 0;
	while (sched_ticks - start < n) {
		spin_sink = s++;             /* CPU-bound, no yield */
	}
}

static void run_cpu_test(int id, const char *name)
{
	unsigned int canary[4];
	int it;

	canary[0] = 0xa5a50000u | (unsigned int) id;
	canary[1] = 0x5a5a0000u | (unsigned int) id;
	canary[2] = 0x12345678u;
	canary[3] = 0x9abcdef0u;

	for (it = 1; it <= TEST_ITERATIONS; it++) {
		if (id == 1 && it == 1) {
			/* TEST-B: wait until SYSTEM is indefinitely suspended, then
			 * reactivate it through the job id. */
			while (jobs[JOB_SYSTEM].status >= 0)
				spin_ticks(1);
			qtrap(QT_REACTIVATE, (unsigned int) test_system_id, 0, 0);
		}
		spin_ticks(TEST_SLICE_TICKS);
		if (canary[0] != (0xa5a50000u | (unsigned int) id) ||
		    canary[1] != (0x5a5a0000u | (unsigned int) id) ||
		    canary[2] != 0x12345678u || canary[3] != 0x9abcdef0u)
			test_results[id].canary_errors++;
		test_results[id].iterations = it;
		if (id == 0 && it == 1) {
			/* Cooperative switch: callee-saved frame, resumed later. */
			yield_asm();
			test_results[id].coop_resumed = 1;
			con_puts("TEST-A: resumed after cooperative yield\n");
		}
		con_puts(name);
		con_puts(": iteration ");
		con_putdec((unsigned long) it);
		con_putc('\n');
	}
	con_puts(name);
	con_puts(": exited\n");
	test_results[id].preempts = jobs[job_current].preempts;
	test_results[id].exited = 1;
}

void test_job_a(void)
{
	run_cpu_test(0, "TEST-A");
}

void test_job_b(void)
{
	run_cpu_test(1, "TEST-B");
}

/* Short job that reuses TEST-A's slot. */
void test_job_c(void)
{
	con_puts("TEST-C: running in reused slot\n");
	test_results[2].iterations = 1;
	test_results[2].exited = 1;
	con_puts("TEST-C: exited\n");
}
