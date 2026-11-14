/*
 * QDOS/386 R3 - guest memory tests (see test_memory.h).
 *
 * Preemption model.  Allocation and free happen inside service traps, and
 * the trap gate runs with IF clear, so the timer cannot interrupt an
 * allocator operation.  The memory jobs are preempted between traps, while
 * they hold blocks.  The test checks that the heap and their blocks are
 * still consistent after each preemption.
 */
#include "platform.h"
#include "jobs.h"
#include "memory.h"
#include "channel.h"
#include "test_jobs.h"
#include "test_memory.h"

#define M1_ITER 4                 /* job MEM-1: blocks held across preemption */
#define M2_ITER 12                /* job MEM-2: allocation churn */
#define M1_SLOT_RESULT 3
#define M2_SLOT_RESULT 4
#define M3_SLOT_RESULT 5
#define S_FIRST_RESULT 6          /* short jobs use results 6..9 */
#define HEAP_PTRS 2048

struct r3_check r3_checks[R3_MAX_CHECKS];
int r3_nchecks = 0;
volatile int r3_phase_done = 0;

static unsigned int m1_leftover = 0;     /* block left allocated by MEM-1 */
static unsigned int m3_ptr = 0;          /* block held by MEM-3 */
static volatile int m3_ready = 0;
static unsigned char m3_flag = 0;
static unsigned int s_leftover[TEST_RESULTS];
static int s_round = 0;

void r3_check(const char *name, int ok)
{
	if (r3_nchecks < R3_MAX_CHECKS) {
		r3_checks[r3_nchecks].name = name;
		r3_checks[r3_nchecks].ok = ok ? 1 : 0;
		r3_nchecks++;
	}
}

static void fill(unsigned int addr, unsigned int n, unsigned char v)
{
	unsigned char *p = (unsigned char *) addr;
	while (n-- != 0)
		*p++ = v;
}

static int check_fill(unsigned int addr, unsigned int n, unsigned char v)
{
	unsigned char *p = (unsigned char *) addr;
	while (n-- != 0)
		if (*p++ != v)
			return 0;
	return 1;
}

static unsigned char pat(int it, int k)
{
	return (unsigned char) (0x40 + it * 7 + k * 13);
}

static int is_err(unsigned int v, int code)
{
	return (int) v == code;
}

/* ---------------------------------------------------------------- heap */

void memory_heap_tests(void)
{
	static unsigned int ptrs[HEAP_PTRS];
	struct mem_stats st0, st1, st;
	unsigned int sizes[9];
	unsigned int p[16];
	unsigned int q[8];
	int ok, i, j, n, rc;
	unsigned int s, k;
	unsigned char *c;

	sizes[0] = 1;   sizes[1] = 15;  sizes[2] = 16;  sizes[3] = 17;
	sizes[4] = 100; sizes[5] = 255; sizes[6] = 256; sizes[7] = 1000;
	sizes[8] = 4096;

	/* H1: alignment, zero-fill, recorded size.  Each block is written, then
	 * freed, so its contents are checked on the next allocation. */
	ok = 1;
	mem_stats(&st0);
	for (i = 0; i < 9; i++) {
		unsigned int a = (unsigned int) mem_alloc(sizes[i], MEM_OWNER_KERNEL);
		if (mem_is_error(a)) {
			ok = 0;
			continue;
		}
		if (a & (MEM_ALIGN - 1))
			ok = 0;
		if (!check_fill(a, sizes[i], 0))
			ok = 0;
		fill(a, sizes[i], 0xa5);
		ptrs[i] = a;
	}
	for (i = 0; i < 9; i++)
		if (mem_free((void *) ptrs[i], MEM_OWNER_KERNEL) != MEM_OK)
			ok = 0;
	mem_stats(&st1);
	if (st1.free != st0.free || st1.free_blocks != st0.free_blocks)
		ok = 0;
	r3_check("heap alignment, zero-fill, reuse", ok);

	/* recorded size: one 100-byte block costs round16(100) + 16 = 128 bytes */
	mem_stats(&st0);
	p[0] = (unsigned int) mem_alloc(100, MEM_OWNER_KERNEL);
	mem_stats(&st1);
	ok = (st1.used - st0.used == 128);
	mem_free((void *) p[0], MEM_OWNER_KERNEL);
	r3_check("block size recorded as rounded size", ok);

	/* H2: non-overlap and canaries.  Blocks are filled with distinct
	 * patterns, then all are checked again after every allocation. */
	ok = 1;
	n = 16;
	for (i = 0; i < n; i++) {
		s = 48 + 16 * (i % 5);
		p[i] = (unsigned int) mem_alloc(s, MEM_OWNER_KERNEL);
		if (mem_is_error(p[i])) {
			ok = 0;
			n = i;
			break;
		}
		fill(p[i], s, (unsigned char) (i + 1));
		for (j = 0; j < i; j++) {
			unsigned int sj = 48 + 16 * (j % 5);
			if (!check_fill(p[j], sj, (unsigned char) (j + 1)))
				ok = 0;
		}
	}
	for (i = 0; i < n; i++)
		for (j = i + 1; j < n; j++) {
			unsigned int si = 48 + 16 * (i % 5);
			unsigned int sj = 48 + 16 * (j % 5);
			if (p[i] < p[j] + sj && p[j] < p[i] + si)
				ok = 0;
		}
	for (i = 0; i < n; i++)
		if (mem_free((void *) p[i], MEM_OWNER_KERNEL) != MEM_OK)
			ok = 0;
	r3_check("non-overlapping blocks keep their canaries", ok && n == 16);

	/* H3: coalescing, forward then reverse.  Eight 128-byte blocks; the
	 * even ones are freed first so the odd ones are sandwiched. */
	ok = 1;
	mem_stats(&st0);
	for (i = 0; i < 8; i++)
		q[i] = (unsigned int) mem_alloc(128, MEM_OWNER_KERNEL);
	for (i = 0; i < 8; i += 2)
		if (mem_free((void *) q[i], MEM_OWNER_KERNEL) != MEM_OK || mem_validate() != MEM_OK)
			ok = 0;
	for (i = 1; i < 8; i += 2)
		if (mem_free((void *) q[i], MEM_OWNER_KERNEL) != MEM_OK || mem_validate() != MEM_OK)
			ok = 0;
	mem_stats(&st);
	if (st.free != st0.free || st.free_blocks != 1)
		ok = 0;
	for (i = 0; i < 8; i++)
		q[i] = (unsigned int) mem_alloc(128, MEM_OWNER_KERNEL);
	for (i = 7; i >= 0; i--)
		if (mem_free((void *) q[i], MEM_OWNER_KERNEL) != MEM_OK || mem_validate() != MEM_OK)
			ok = 0;
	mem_stats(&st);
	if (st.free != st0.free || st.free_blocks != 1)
		ok = 0;
	r3_check("adjacent free blocks coalesce (both orders)", ok);

	/* H4: exhaustion.  Fill with 4 KiB blocks until one fails, then check
	 * the accounting, the exact-fit reuse and the full release. */
	mem_stats(&st0);
	n = 0;
	rc = MEM_OK;
	while (n < HEAP_PTRS) {
		unsigned int a = (unsigned int) mem_alloc(4096, MEM_OWNER_KERNEL);
		if (mem_is_error(a)) {
			rc = (int) a;
			break;
		}
		ptrs[n++] = a;
	}
	mem_stats(&st);
	ok = (rc == MEM_ERR_NOMEM) && (n > 0) && (st.used + st.free == st.total) &&
	     (st.largest_free < 4096 + 16);
	if (n > 0) {
		/* exact-fit reuse: free one block and take the same size again */
		unsigned int last = ptrs[n - 1];
		mem_free((void *) last, MEM_OWNER_KERNEL);
		/* The freed block may merge with a free neighbour, so the new address
		 * is not necessarily 'last'; only success is required. */
		k = (unsigned int) mem_alloc(4096, MEM_OWNER_KERNEL);
		if (mem_is_error(k))
			ok = 0;
		else
			ptrs[n - 1] = k;             /* the new block is now the last one */
	}
	for (i = 0; i < n; i++)
		if (mem_free((void *) ptrs[i], MEM_OWNER_KERNEL) != MEM_OK)
			ok = 0;
	mem_stats(&st);
	if (st.free != st0.free || st.free_blocks != 1 || mem_validate() != MEM_OK)
		ok = 0;
	r3_check("exhaustion: clean NOMEM, exact fit, full release", ok);

	/* H5: zero size, overflow, and oversize requests */
	mem_stats(&st);
	ok = is_err((unsigned int) mem_alloc(0, MEM_OWNER_KERNEL), MEM_ERR_ZERO) &&
	     is_err((unsigned int) mem_alloc(0xffffffffu, MEM_OWNER_KERNEL), MEM_ERR_OVERFLOW) &&
	     is_err((unsigned int) mem_alloc(0xffffffe1u, MEM_OWNER_KERNEL), MEM_ERR_OVERFLOW) &&
	     is_err((unsigned int) mem_alloc(st.total + 1, MEM_OWNER_KERNEL), MEM_ERR_NOMEM);
	r3_check("zero size, overflow and oversize rejected", ok);

	/* H6: invalid pointers, wrong owner, double free */
	ok = 1;
	p[0] = (unsigned int) mem_alloc(64, MEM_OWNER_KERNEL);
	c = (unsigned char *) p[0];
	if (mem_free((void *) 0, MEM_OWNER_KERNEL) != MEM_ERR_BADPTR)
		ok = 0;
	if (mem_free((void *) (p[0] + 1), MEM_OWNER_KERNEL) != MEM_ERR_BADPTR)
		ok = 0;
	if (mem_free((void *) (p[0] + 16), MEM_OWNER_KERNEL) != MEM_ERR_BADPTR)
		ok = 0;
	if (mem_free((void *) p[0], 7) != MEM_ERR_OWNER)
		ok = 0;
	if (mem_free((void *) p[0], MEM_OWNER_KERNEL) != MEM_OK)
		ok = 0;
	if (mem_free((void *) p[0], MEM_OWNER_KERNEL) != MEM_ERR_DOUBLE)
		ok = 0;
	(void) c;
	r3_check("bad pointer, wrong owner and double free rejected", ok);

	r3_check("heap invariants after heap tests", mem_validate() == MEM_OK);
}

/* ------------------------------------------------------------ job tests */

/* MEM-1: CPU-bound.  Holds blocks across preemption, checks their contents,
 * frees most, and leaves one block allocated for the reclaim test. */
void test_job_m1(void)
{
	struct test_result *r = &test_results[M1_SLOT_RESULT];
	unsigned int canary[4];
	unsigned int ptr[6];
	int it, k;

	canary[0] = 0xa1a10001u;
	canary[1] = 0xa1a10002u;
	canary[2] = 0xa1a10003u;
	canary[3] = 0xa1a10004u;

	for (it = 1; it <= M1_ITER; it++) {
		for (k = 0; k < 6; k++) {
			unsigned int s = 96 + 48 * k + 16 * it;
			int rv = qtrap(QT_MEM_ALLOC, s, 0, 0);
			ptr[k] = (unsigned int) rv;
			if (rv <= 0) {
				r->canary_errors++;
				ptr[k] = 0;
			} else {
				fill(ptr[k], s, pat(it, k));
			}
		}
		spin_ticks(1);                     /* preempted while holding blocks */
		for (k = 0; k < 6; k++) {
			unsigned int s = 96 + 48 * k + 16 * it;
			if (ptr[k] != 0 && !check_fill(ptr[k], s, pat(it, k)))
				r->canary_errors++;
		}
		for (k = 0; k < 5; k++)
			if (ptr[k] != 0 && qtrap(QT_MEM_FREE, ptr[k], 0, 0) != 0)
				r->canary_errors++;
		if (it == M1_ITER)
			m1_leftover = ptr[5];          /* never freed: reclaimed at exit */
		else if (ptr[5] != 0 && qtrap(QT_MEM_FREE, ptr[5], 0, 0) != 0)
			r->canary_errors++;
		if (canary[0] != 0xa1a10001u || canary[1] != 0xa1a10002u ||
		    canary[2] != 0xa1a10003u || canary[3] != 0xa1a10004u)
			r->canary_errors++;
		r->iterations = it;
	}
	r->preempts = jobs[job_current].preempts;
	r->exited = 1;
}

/* MEM-2: allocation churn across preemption; frees everything before exit. */
void test_job_m2(void)
{
	struct test_result *r = &test_results[M2_SLOT_RESULT];
	unsigned int ptr[8];
	int it, k;

	for (it = 1; it <= M2_ITER; it++) {
		for (k = 0; k < 8; k++) {
			unsigned int s = 32 + 56 * k + it;
			int rv = qtrap(QT_MEM_ALLOC, s, 0, 0);
			ptr[k] = (unsigned int) rv;
			if (rv <= 0) {
				r->canary_errors++;
				ptr[k] = 0;
			} else {
				fill(ptr[k], s, pat(it, k));
			}
		}
		if ((it & 3) == 0)
			spin_ticks(1);
		for (k = 0; k < 8; k++) {
			unsigned int s = 32 + 56 * k + it;
			if (ptr[k] != 0) {
				if (!check_fill(ptr[k], s, pat(it, k)))
					r->canary_errors++;
				if (qtrap(QT_MEM_FREE, ptr[k], 0, 0) != 0)
					r->canary_errors++;
			}
		}
		r->iterations = it;
	}
	r->preempts = jobs[job_current].preempts;
	r->exited = 1;
}

/* MEM-3: holds one block while suspended indefinitely; SYSTEM checks the
 * owner rules against it, then releases it.  It frees the block itself. */
void test_job_m3(void)
{
	struct test_result *r = &test_results[M3_SLOT_RESULT];
	unsigned int s = 256;
	int rv;

	rv = qtrap(QT_MEM_ALLOC, s, 0, 0);
	if (rv <= 0) {
		r->canary_errors++;
	} else {
		m3_ptr = (unsigned int) rv;
		fill(m3_ptr, s, 0x3c);
	}
	m3_ready = 1;
	m3_flag = 0;
	qtrap(QT_SUSPEND, (unsigned int) -1, (unsigned int) &m3_flag, 0);
	if (m3_ptr != 0) {
		if (!check_fill(m3_ptr, s, 0x3c))
			r->canary_errors++;
		if (qtrap(QT_MEM_FREE, m3_ptr, 0, 0) != 0)
			r->canary_errors++;
	}
	r->iterations = 1;
	r->preempts = jobs[job_current].preempts;
	r->exited = 1;
}

/* Short job for the reuse and pressure tests.  Its result slot is s_round. */
void test_job_s(void)
{
	int idx = S_FIRST_RESULT + s_round;
	int rv = qtrap(QT_MEM_ALLOC, 128 + 16 * s_round, 0, 0);
	s_leftover[idx] = (unsigned int) rv;  /* left allocated: reclaimed at exit */
	test_results[idx].iterations = 1;
	test_results[idx].exited = 1;
}

/* ---------------------------------------------------- SYSTEM choreography */

static void wait_exit(int idx)
{
	while (!test_results[idx].exited)
		qtrap(QT_SUSPEND, 1, 0, 0);
}

/* Wait for the job's slot to be reclaimed, then record its high-water mark. */
static void reap(int idx, unsigned int id)
{
	int slot = (int) (id & 0xffffu);
	wait_exit(idx);
	while (jobs[slot].in_use)
		qtrap(QT_SUSPEND, 1, 0, 0);
	test_results[idx].hwm = jobs[slot].stack_hwm;
}

static int slots_in_use(void)
{
	int i, n = 0;
	for (i = 1; i < JOB_MAX; i++)
		if (jobs[i].in_use)
			n++;
	return n;
}

void memory_job_phase(void)
{
	struct mem_stats st;
	unsigned int base_free;
	unsigned int fillers[HEAP_PTRS];
	unsigned int id1, id2, id3, id_s[4];
	int rc, i, n, before, after, ok;
	unsigned int rr;

	/* R2's TEST-B and TEST-C are finished; their slots must be reclaimed
	 * before the memory jobs start (slots 1 and 2). */
	while (!test_results[1].exited)
		qtrap(QT_SUSPEND, 1, 0, 0);
	while (!test_results[2].exited)
		qtrap(QT_SUSPEND, 1, 0, 0);
	while (jobs[1].in_use || jobs[2].in_use)
		qtrap(QT_SUSPEND, 1, 0, 0);

	/* R11E: only the owner of a block may free it, and the user range starts at
	 * the heap (kernel memory below it is not user memory). */
	{
		void *blk = mem_alloc(64, MEM_OWNER_KERNEL);
		r3_check("free by a different owner is refused",
		         blk != 0 && mem_free(blk, 1234u) == MEM_ERR_OWNER);
		r3_check("free by the owner succeeds",
		         blk != 0 && mem_free(blk, MEM_OWNER_KERNEL) == MEM_OK);
		r3_check("user range refuses kernel memory",
		         chan_user_range_ok(0x00100000u, 1) == 0);
		r3_check("user range accepts the heap base",
		         chan_user_range_ok(mem_arena_base(), 1) == 1);
	}

	base_free = (unsigned int) qtrap(QT_MEM_FREE_BYTES, 0, 0, 0);

	/* A. Two preempted allocating jobs, then reclaim and the heap restored. */
	id1 = (unsigned int) qtrap(QT_CREATE, (unsigned int) test_job_m1, 3, (unsigned int) "MEM-1");
	id2 = (unsigned int) qtrap(QT_CREATE, (unsigned int) test_job_m2, 3, (unsigned int) "MEM-2");
	reap(M1_SLOT_RESULT, id1);
	reap(M2_SLOT_RESULT, id2);
	r3_check("MEM-1 and MEM-2 preempted while allocating",
	         test_results[M1_SLOT_RESULT].preempts > 0 &&
	         test_results[M2_SLOT_RESULT].preempts > 0);
	r3_check("MEM-1 and MEM-2 contents intact across preemption",
	         test_results[M1_SLOT_RESULT].canary_errors == 0 &&
	         test_results[M2_SLOT_RESULT].canary_errors == 0 &&
	         test_results[M1_SLOT_RESULT].iterations == M1_ITER &&
	         test_results[M2_SLOT_RESULT].iterations == M2_ITER);
	r3_check("dynamic stacks: high-water mark recorded",
	         test_results[M1_SLOT_RESULT].hwm > 0 &&
	         test_results[M1_SLOT_RESULT].hwm < JOB_STACK_BYTES);
	r3_check("job exit reclaims its heap blocks",
	         (unsigned int) qtrap(QT_MEM_FREE_BYTES, 0, 0, 0) == base_free);
	rr = (unsigned int) mem_free((void *) m1_leftover, id1);
	r3_check("reclaimed block: stale owner and pointer rejected",
	         (int) rr == MEM_ERR_DOUBLE || (int) rr == MEM_ERR_BADPTR);

	/* B. Slot reuse with a new tag, stale owner, then release by the owner. */
	m3_ready = 0;
	m3_ptr = 0;
	id3 = (unsigned int) qtrap(QT_CREATE, (unsigned int) test_job_m3, 3, (unsigned int) "MEM-3");
	while (!m3_ready)
		qtrap(QT_SUSPEND, 1, 0, 0);
	r3_check("reused slot gets a new tag",
	         (id3 & 0xffffu) == (id1 & 0xffffu) && id3 != id1);
	r3_check("stale job id cannot reactivate reused slot",
	         qtrap(QT_REACTIVATE, id1, 0, 0) == -2);
	r3_check("stale owner cannot free a live block",
	         mem_free((void *) m3_ptr, id1) == MEM_ERR_OWNER &&
	         mem_free((void *) m3_ptr, MEM_OWNER_KERNEL) == MEM_ERR_OWNER);
	qtrap(QT_REACTIVATE, id3, 0, 0);
	reap(M3_SLOT_RESULT, id3);
	r3_check("owner frees its own block; suspended job resumed",
	         test_results[M3_SLOT_RESULT].canary_errors == 0 &&
	         test_results[M3_SLOT_RESULT].exited == 1);

	/* C. Repeated create, exit and reuse, distinct ids, heap restored. */
	for (i = 0; i < 3; i++) {
		s_round = i;
		id_s[i] = (unsigned int) qtrap(QT_CREATE, (unsigned int) test_job_s, 2, (unsigned int) "MEM-S");
		reap(S_FIRST_RESULT + i, id_s[i]);
	}
	ok = (id_s[0] != id_s[1]) && (id_s[1] != id_s[2]) && (id_s[0] != id_s[2]);
	r3_check("repeated create/exit/reuse: distinct tagged ids", ok);
	r3_check("repeated create/exit/reuse: heap restored",
	         (unsigned int) qtrap(QT_MEM_FREE_BYTES, 0, 0, 0) == base_free);

	/* D. Creation under memory pressure must fail without a leaked slot. */
	n = 0;
	while (n < HEAP_PTRS && mem_stats(&st) == MEM_OK && st.largest_free >= 32) {
		unsigned int a = (unsigned int) mem_alloc(st.largest_free - 16, MEM_OWNER_KERNEL);
		if (mem_is_error(a))
			break;
		fillers[n++] = a;
	}
	mem_stats(&st);
	before = slots_in_use();
	s_round = 3;
	rc = qtrap(QT_CREATE, (unsigned int) test_job_s, 2, (unsigned int) "MEM-X");
	after = slots_in_use();
	r3_check("job creation fails cleanly under memory pressure",
	         rc == MEM_ERR_NOMEM && before == after && st.largest_free < JOB_STACK_BYTES + 16);
	for (i = 0; i < n; i++)
		mem_free((void *) fillers[i], MEM_OWNER_KERNEL);
	r3_check("pressure released: heap restored",
	         (unsigned int) qtrap(QT_MEM_FREE_BYTES, 0, 0, 0) == base_free);
	s_round = 3;
	id_s[3] = (unsigned int) qtrap(QT_CREATE, (unsigned int) test_job_s, 2, (unsigned int) "MEM-S");
	reap(S_FIRST_RESULT + 3, id_s[3]);
	r3_check("creation succeeds once memory is available",
	         (int) id_s[3] >= 0 && test_results[S_FIRST_RESULT + 3].exited == 1);

	/* E. Invariants and stacks after the whole run. */
	r3_check("heap invariants hold after the job phase",
	         qtrap(QT_MEM_VALIDATE, 0, 0, 0) == MEM_OK && mem_validate() == MEM_OK);
	r3_check("no stack canary failure seen by the ISR", stack_faults == 0);
	r3_check("heap restored to baseline at the end",
	         (unsigned int) qtrap(QT_MEM_FREE_BYTES, 0, 0, 0) == base_free);
	r3_phase_done = 1;
}
