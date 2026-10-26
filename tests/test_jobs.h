/*
 * QDOS/386 R1 - CPU-bound test jobs (TEST-A, TEST-B).
 */
#ifndef QDOS386_TEST_JOBS_H
#define QDOS386_TEST_JOBS_H

#define TEST_ITERATIONS 3

struct test_result {
	int iterations;        /* completed iterations */
	int canary_errors;     /* stack values that changed across preemption */
	int coop_resumed;      /* TEST-A: resumed correctly after a cooperative yield */
};

extern struct test_result test_results[2];
void test_job_a(void);
void test_job_b(void);

#endif
