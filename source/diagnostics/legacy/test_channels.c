/*
 * QDOS/386 R4 - guest channel tests (see test_channels.h).
 *
 * The jobs call only the channel service traps, so the console is reached
 * through the channel manager, never by the test calling the platform output
 * routine.  The jobs run under timer preemption.  Checks are invariants, not
 * timing: the number of ticks, preemptions and output order may vary.
 *
 * Slot budget: SYSTEM (0) and IDLE (3) are permanent, so at most two
 * application jobs are alive at once.  CHAN-A and CHAN-B run together, then
 * CHAN-C runs alone after both are reclaimed.
 */
#include "platform.h"
#include "jobs.h"
#include "memory.h"
#include "irq.h"
#include "channel.h"
#include "test_jobs.h"
#include "test_channels.h"

#define MSG "R4 CHANNEL STRING\n"
#define MSG_LEN (sizeof(MSG) - 1)

/* Indices into the CHAN-A result array. */
#define A_OPEN 0
#define A_PUTC1 1
#define A_WRITE_COUNT 2
#define A_CLOSE 3
#define A_WRITE_AFTER 4
#define A_DOUBLE_CLOSE 5
#define A_REUSE_TAG 6
#define A_STALE 7
#define A_OWNER 8
#define A_OWNER_STILL_OPEN 9
#define A_PREEMPT_WRITES 10
#define A_PREEMPTED 11
#define A_RESULTS 12

struct r4_check r4_checks[R4_MAX_CHECKS];
int r4_nchecks = 0;
volatile int r4_phase_done = 0;

static volatile int ares[A_RESULTS];
static volatile int a_done = 0;
static volatile int b_done = 0;
static volatile int c_done = 0;
static volatile int probe_ready = 0;
static volatile int probe_done = 0;
static volatile unsigned int probe_id = 0;
static volatile int probe_rc[3];
static volatile unsigned int c_id = 0;

void r4_add(const char *name, int ok)
{
	if (r4_nchecks < R4_MAX_CHECKS) {
		r4_checks[r4_nchecks].name = name;
		r4_checks[r4_nchecks].ok = ok ? 1 : 0;
		r4_checks[r4_nchecks].na = 0;
		r4_nchecks++;
	}
}

static void r4_add_na(const char *name)
{
	if (r4_nchecks < R4_MAX_CHECKS) {
		r4_checks[r4_nchecks].name = name;
		r4_checks[r4_nchecks].ok = 1;
		r4_checks[r4_nchecks].na = 1;
		r4_nchecks++;
	}
}

/* CHAN-A: owner of a console channel.  Runs the sequence of items 1-9, and
 * the preempted writes of item 12. */
void test_job_chan_a(void)
{
	int id1, id2, rc, i, ok;

	id1 = qtrap(QT_CH_OPEN, (unsigned int) "CON", 0, 0);
	ares[A_OPEN] = (id1 > 0) && ((id1 >> 16) != 0) && ((id1 & 0xffff) < CHAN_MAX);

	rc = qtrap(QT_CH_PUTC, (unsigned int) id1, 'R', 0);
	ares[A_PUTC1] = (rc == 1);

	rc = qtrap(QT_CH_WRITE, (unsigned int) id1, (unsigned int) MSG, MSG_LEN);
	ares[A_WRITE_COUNT] = (rc == (int) MSG_LEN);

	ares[A_CLOSE] = (qtrap(QT_CH_CLOSE, (unsigned int) id1, 0, 0) == 0);

	ares[A_WRITE_AFTER] =
		(qtrap(QT_CH_WRITE, (unsigned int) id1, (unsigned int) MSG, 4) == CHAN_ERR_BADID) &&
		(qtrap(QT_CH_PUTC, (unsigned int) id1, 'x', 0) == CHAN_ERR_BADID);

	ares[A_DOUBLE_CLOSE] = (qtrap(QT_CH_CLOSE, (unsigned int) id1, 0, 0) == CHAN_ERR_BADID);

	/* Reopen: the lowest free slot again, but with a new tag. */
	id2 = qtrap(QT_CH_OPEN, (unsigned int) "CON", 0, 0);
	ares[A_REUSE_TAG] = (id2 > 0) &&
		((id2 & 0xffff) == (id1 & 0xffff)) && ((id2 >> 16) != (id1 >> 16));

	ares[A_STALE] =
		(qtrap(QT_CH_PUTC, (unsigned int) id1, 'x', 0) == CHAN_ERR_BADID) &&
		(qtrap(QT_CH_CLOSE, (unsigned int) id1, 0, 0) == CHAN_ERR_BADID);

	/* Publish the open channel to CHAN-B and wait for its attempts. */
	probe_id = (unsigned int) id2;
	probe_ready = 1;
	while (!probe_done)
		qtrap(QT_SUSPEND, 1, 0, 0);
	ares[A_OWNER] = (probe_rc[0] == CHAN_ERR_OWNER) &&
		(probe_rc[1] == CHAN_ERR_OWNER) && (probe_rc[2] == CHAN_ERR_OWNER);
	ares[A_OWNER_STILL_OPEN] = (qtrap(QT_CH_PUTC, (unsigned int) id2, '!', 0) == 1);

	/* Writes spread over several ticks, so the timer preempts this job
	 * between them. */
	ok = 1;
	for (i = 0; i < 4; i++) {
		spin_ticks(2);
		if (qtrap(QT_CH_PUTC, (unsigned int) id2, '.', 0) != 1)
			ok = 0;
	}
	qtrap(QT_CH_PUTC, (unsigned int) id2, '\n', 0);
	ares[A_PREEMPT_WRITES] = ok;
	ares[A_PREEMPTED] = (jobs[job_current].preempts > 0);

	(void) qtrap(QT_CH_CLOSE, (unsigned int) id2, 0, 0);
	a_done = 1;
}

/* CHAN-B: attempts to use CHAN-A's open channel.  Every attempt must fail
 * with CHAN_ERR_OWNER, and none may close it. */
void test_job_chan_b(void)
{
	while (!probe_ready)
		qtrap(QT_SUSPEND, 1, 0, 0);
	probe_rc[0] = qtrap(QT_CH_PUTC, probe_id, 'x', 0);
	probe_rc[1] = qtrap(QT_CH_WRITE, probe_id, (unsigned int) MSG, 4);
	probe_rc[2] = qtrap(QT_CH_CLOSE, probe_id, 0, 0);
	probe_done = 1;
	b_done = 1;
}

/* CHAN-C: opens a channel, writes, and exits without closing it. */
void test_job_chan_c(void)
{
	int id;
	id = qtrap(QT_CH_OPEN, (unsigned int) "CON", 0, 0);
	c_id = (unsigned int) id;
	qtrap(QT_CH_WRITE, (unsigned int) id, (unsigned int) MSG, MSG_LEN);
	c_done = 1;
}

/* IRQ classification checks, run in SYSTEM context.  The software injections
 * raise vector 0x27 and vector 0x2F with no hardware request, so no ISR bit is
 * set: both must be classified as spurious.  The genuine branch depends on a
 * slave IRQ15 that this platform cannot raise (the slave is fully masked), so
 * it is checked only through the pure rule. */
static void pic_checks(void)
{
	unsigned long m7, s15;

	m7 = irq_spurious_master7;
	sw_int27();
	r4_add("IRQ7 (0x27): injected interrupt classified spurious",
	       irq_spurious_master7 == m7 + 1 && irq_genuine_master7 == 0);

	s15 = irq_spurious_slave15;
	sw_int2f();
	r4_add("IRQ15 (0x2F): injected interrupt classified spurious",
	       irq_spurious_slave15 == s15 + 1 && irq_genuine_slave15 == 0);

	if (pic_present()) {
		r4_add("PIC in-service registers clear after spurious injections",
		       pic_master_isr() == 0 && pic_slave_isr() == 0);
	} else {
		r4_add_na("PIC in-service registers clear after spurious injections");
	}

	r4_add("ISR rule: bit 7 set means genuine, clear means spurious",
	       irq_isr_bit_set(0x80, 7) == 1 && irq_isr_bit_set(0x7f, 7) == 0 &&
	       irq_isr_bit_set(0x04, 2) == 1 && irq_isr_bit_set(0x00, 2) == 0);
}

static void wait_reclaimed(int id)
{
	if (id <= 0)
		return;
	while (jobs[id & 0xffff].in_use)
		qtrap(QT_SUSPEND, 1, 0, 0);
}

/* R5: exit status records (QT_EXIT, QT_JOB_EXIT).  Run inside the channel
 * phase, after CHAN-A, CHAN-B and CHAN-C have been reclaimed. */
static volatile int x_a_ran, x_b_ran, x_r_go, x_r_done;

static void test_job_exit_a(void)
{
	x_a_ran = 1;
	qtrap(QT_EXIT, 42, 0, 0);
}

static void test_job_exit_b(void)
{
	x_b_ran = 1;                  /* returns without QT_EXIT */
}

static void test_job_exit_r(void)
{
	while (!x_r_go)
		qtrap(QT_SUSPEND, 1, 0, 0);
	x_r_done = 1;
}

static void exit_job_phase(void)
{
	int idA, idB, idR, idN, st;

	/* Only two job slots are free here (JOB_MAX 4), so the jobs run in
	 * sequence: A and B first, then R, then N in the reused slot. */
	x_a_ran = 0; x_b_ran = 0; x_r_go = 0; x_r_done = 0;
	idA = qtrap(QT_CREATE, (unsigned int) test_job_exit_a, 2, (unsigned int) "EXIT-A");
	idB = qtrap(QT_CREATE, (unsigned int) test_job_exit_b, 2, (unsigned int) "EXIT-B");
	if (idA <= 0 || idB <= 0) {
		r4_add("exit jobs A and B created", 0);
		wait_reclaimed(idA);
		wait_reclaimed(idB);
		return;
	}
	while (!x_a_ran || !x_b_ran)
		qtrap(QT_SUSPEND, 1, 0, 0);
	wait_reclaimed(idA);
	wait_reclaimed(idB);
	st = (int) qtrap(QT_JOB_EXIT, (unsigned int) idA, 0, 0);
	r4_add("QT_EXIT status recorded and readable after reclaim", st == 42);
	st = (int) qtrap(QT_JOB_EXIT, (unsigned int) idB, 0, 0);
	r4_add("job ending without QT_EXIT gives JOB_ERR_NOSTATUS", st == JOB_ERR_NOSTATUS);
	st = (int) qtrap(QT_JOB_EXIT, (3u << 16) | 3u, 0, 0);
	r4_add("never-created job id gives -2", st == -2);

	idR = qtrap(QT_CREATE, (unsigned int) test_job_exit_r, 2, (unsigned int) "EXIT-R");
	if (idR <= 0) {
		r4_add("exit job R created", 0);
		return;
	}
	st = (int) qtrap(QT_JOB_EXIT, (unsigned int) idR, 0, 0);
	r4_add("running job: QT_JOB_EXIT gives JOB_ERR_RUNNING", st == JOB_ERR_RUNNING);
	x_r_go = 1;
	while (!x_r_done)
		qtrap(QT_SUSPEND, 1, 0, 0);
	wait_reclaimed(idR);

	/* Reuse: the next job takes the lowest free slot, so the old id of that
	 * slot goes stale and the other record is kept. */
	idN = qtrap(QT_CREATE, (unsigned int) test_job_exit_b, 2, (unsigned int) "EXIT-N");
	if (idN <= 0) {
		r4_add("exit job N created", 0);
		return;
	}
	{
		int qa = (int) qtrap(QT_JOB_EXIT, (unsigned int) idA, 0, 0);
		int qb = (int) qtrap(QT_JOB_EXIT, (unsigned int) idB, 0, 0);
		unsigned int sn = (unsigned int) idN & 0xffffu;
		int ok = 0;
		if (sn == ((unsigned int) idA & 0xffffu))
			ok = (qa == -2 && qb == JOB_ERR_NOSTATUS);
		else if (sn == ((unsigned int) idB & 0xffffu))
			ok = (qb == -2 && qa == 42);
		r4_add("slot reuse: reused slot's old id is stale, other record kept", ok);
	}
	while (!x_b_ran)
		qtrap(QT_SUSPEND, 1, 0, 0);
	wait_reclaimed(idN);
}

void channel_job_phase(void)
{
	struct mem_stats st;
	unsigned long base_free, base_live, closed0, reclaimed0;
	int idA, idB, idC;
	int i;

	/* Baseline before any channel job exists. */
	mem_stats(&st);
	base_free = st.free;
	base_live = chan_objects_live();
	closed0 = chan_closed_on_exit;
	reclaimed0 = jobs_reclaimed;

	pic_checks();

	a_done = 0; b_done = 0; c_done = 0;
	probe_ready = 0; probe_done = 0;
	for (i = 0; i < A_RESULTS; i++)
		ares[i] = 0;

	idA = qtrap(QT_CREATE, (unsigned int) test_job_chan_a, 2, (unsigned int) "CHAN-A");
	idB = qtrap(QT_CREATE, (unsigned int) test_job_chan_b, 2, (unsigned int) "CHAN-B");
	if (idA <= 0 || idB <= 0) {
		r4_add("CHAN-A and CHAN-B created", 0);
	} else {
		while (!a_done || !b_done)
			qtrap(QT_SUSPEND, 1, 0, 0);
		wait_reclaimed(idA);
		wait_reclaimed(idB);
	}

	idC = qtrap(QT_CREATE, (unsigned int) test_job_chan_c, 2, (unsigned int) "CHAN-C");
	if (idC <= 0) {
		r4_add("CHAN-C created", 0);
	} else {
		while (!c_done)
			qtrap(QT_SUSPEND, 1, 0, 0);
		wait_reclaimed(idC);
	}

	exit_job_phase();
	r4_add("open console channel: valid tagged id", ares[A_OPEN]);
	r4_add("write one character returns 1", ares[A_PUTC1]);
	r4_add("write string returns verified byte count", ares[A_WRITE_COUNT]);
	r4_add("close channel returns 0", ares[A_CLOSE]);
	r4_add("write after close rejected", ares[A_WRITE_AFTER]);
	r4_add("double close rejected", ares[A_DOUBLE_CLOSE]);
	r4_add("reopen reuses the slot with a new tag", ares[A_REUSE_TAG]);
	r4_add("stale channel id rejected", ares[A_STALE]);
	r4_add("other job cannot write, putc or close an owned channel", ares[A_OWNER]);
	r4_add("owner's channel still open after rejected attempts", ares[A_OWNER_STILL_OPEN]);
	r4_add("channel writes complete under timer preemption", ares[A_PREEMPT_WRITES]);
	r4_add("CHAN-A was preempted while using its channel", ares[A_PREEMPTED]);

	/* Automatic cleanup: CHAN-C exited with its channel open. */
	r4_add("job exit closes its open channel", chan_closed_on_exit == closed0 + 1);
	r4_add("exited job's channel id is stale",
	       qtrap(QT_CH_PUTC, c_id, 'x', 0) == CHAN_ERR_BADID);
	r4_add("channel objects: none live after the channel jobs",
	       chan_objects_live() == base_live);
	mem_stats(&st);
	r4_add("heap restored after the channel phase", st.free == base_free);
	r4_add("channel and exit jobs reclaimed (CHAN-A/B/C, EXIT-A/B/R/N)",
	       jobs_reclaimed == reclaimed0 + 3 + 4);
	r4_add("heap invariants hold after the channel phase", mem_validate() == MEM_OK);

}
