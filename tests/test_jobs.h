/*
 * QDOS/386 R1 - application test jobs.
 *   TEST-A, TEST-B: CPU-bound, preempted by the timer.  TEST-A yields once.
 *   TEST-B waits for SYSTEM to suspend, then reactivates it.
 *   TEST-C: short-lived job that reuses TEST-A's slot.
 */
#ifndef QDOS386_TEST_JOBS_H
#define QDOS386_TEST_JOBS_H

#define TEST_ITERATIONS 3
#define TEST_SLICE_TICKS 4        /* each iteration spans this many ticks */

struct test_result {
	int iterations;               /* completed iterations */
	int canary_errors;            /* stack values changed across preemption */
	int coop_resumed;             /* TEST-A: resumed after its cooperative yield */
	int exited;                   /* job reached its exit point */
	unsigned long preempts;       /* timer preemptions, recorded at exit */
};

extern struct test_result test_results[3];
extern unsigned int test_flag_rel;        /* release flag used by SYSTEM */

void test_job_a(void);
void test_job_b(void);
void test_job_c(void);

#endif
