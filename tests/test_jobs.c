/*
 * QDOS/386 R1 - CPU-bound test jobs.  Neither job yields.  Progress depends
 * on timer preemption.  Each job keeps canary values in its own stack frame
 * and checks them after every iteration, to show that each stack survives
 * preemption.
 */
#include "platform.h"
#include "test_jobs.h"
#include "jobs.h"

#define TEST_SPIN 20000

struct test_result test_results[2];
static volatile unsigned int spin_sink;

static void run_test(int id, const char *name)
{
	unsigned int canary[4];
	unsigned int it, s;

	canary[0] = 0xa5a50000u | (unsigned int) id;
	canary[1] = 0x5a5a0000u | (unsigned int) id;
	canary[2] = 0x12345678u;
	canary[3] = 0x9abcdef0u;

	for (it = 1; it <= TEST_ITERATIONS; it++) {
		for (s = 0; s < TEST_SPIN; s++)
			spin_sink = s;                 /* CPU-bound, no yield */
		if (canary[0] != (0xa5a50000u | (unsigned int) id) ||
		    canary[1] != (0x5a5a0000u | (unsigned int) id) ||
		    canary[2] != 0x12345678u || canary[3] != 0x9abcdef0u)
			test_results[id].canary_errors++;
		test_results[id].iterations = (int) it;
		if (id == 0 && it == 1) {
			/* Cooperative switch: save callee-saved frame, resume later. */
			yield_asm();
			test_results[id].coop_resumed = 1;
			con_puts("TEST-A: resumed after cooperative yield\n");
		}
		con_puts(name);
		con_puts(": iteration ");
		con_putdec(it);
		con_putc('\n');
	}
	con_puts(name);
	con_puts(": exited\n");
}

void test_job_a(void)
{
	run_test(0, "TEST-A");
}

void test_job_b(void)
{
	run_test(1, "TEST-B");
}
