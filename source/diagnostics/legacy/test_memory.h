/*
 * QDOS/386 R3 - guest memory tests.
 *
 * Two groups run inside the guest on both targets:
 *   1. heap tests (memory_heap_tests): kernel-level calls to mem_* before any
 *      job exists.  Covers items 1-6 of the R3 continuation plan.
 *   2. job-phase tests (memory_job_phase): run by SYSTEM after the R2
 *      sequence.  Covers preemption while allocating, reclaim on exit, slot
 *      reuse with new tags, stale and wrong owners, creation under memory
 *      pressure, and stack canaries.
 *
 * Every result is recorded with r3_check and printed by the IDLE job.  The
 * final PASS line is printed only when every check passed.
 */
#ifndef QDOS386_TEST_MEMORY_H
#define QDOS386_TEST_MEMORY_H

#define R3_MAX_CHECKS 40
#define M1_SLOT_RESULT_IDX 3        /* test_results index of MEM-1 (for the report) */

struct r3_check {
	const char *name;
	int ok;
};

extern struct r3_check r3_checks[R3_MAX_CHECKS];
extern int r3_nchecks;
extern volatile int r3_phase_done;

void r3_check(const char *name, int ok);
void memory_heap_tests(void);
void memory_job_phase(void);

void test_job_m1(void);
void test_job_m2(void);
void test_job_m3(void);
void test_job_s(void);

#endif
