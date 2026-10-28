/* R11K TWICET - the acceptance driver for the paged launch path.  An ordinary slot-1 program run
 * from the shell (so it is unpaged), built with the unchanged SDK.  It uses only the public
 * libqdos calls (qd_pg_start, qd_pg_cancel, qd_pginfo, qd_job_*), never a raw trap, except in the
 * negative cases that need a malformed request.  The same file is also the paged "child" for the
 * refusal, fault, console and shared-heap cases (mode C), started by itself - one more proof that
 * a single fixed-origin file runs both ways.
 *
 *   TWICET M   main: two TWICE copies started held (nothing runs), activated together, overlap,
 *              independent state (exit 0x41/0x42), reclamation, stale ids, then relaunch
 *   TWICET H   held starts: cancel, group rollback (all-or-nothing), cancel of a live job refused
 *   TWICET N   negative starts: missing file, malformed header, wrong origin, bad flags, bad args,
 *              a paged caller's refusals (child R)
 *   TWICET E   exhaustion: start held copies until the kernel refuses; rollback, then recovery
 *   TWICET S   repeated launch/exit cycles: no leak, frames reused
 *   TWICET T   first start fails inside the copy (test kernel -DQDOS_TEST_FAULT_LAUNCH=3): rollback
 *   TWICET F   faults in a paged copy (page fault, divide error, invalid opcode) beside a healthy
 *              copy and beside an unpaged job: only the faulting copy ends   (Tiny386 / QEMU only)
 *   TWICET W   console: one paged copy waits for input, a second is refused (-50), then released
 *   TWICET Z   shared heap: copy A writes a kernel-heap block, copy B reads it (NOT isolation)
 *   TWICET K   unpaged poll/schedule tasks beside paged jobs (test kernel -DQDOS_TEST_PG_TASKS)
 *   TWICET Y   review regression (Tiny386/QEMU, ENDS WITH A MACHINE HALT): an UNPAGED poll task at an address
 *              inside the window faults while a paged copy is current; the kernel must halt ("PGFAULT: halt")
 *              and must NOT blame and end the innocent paged copy.  Run last, alone.
 *   TWICET X   non-paging kernel: every paging call answers QD_PG_ERR_NOPAGING (-70); NA on a paging kernel
 *   TWICET C x paged child (x = R refusals, F/D/U fault, W console, Z shared heap)
 * Prints "TWICET <mode>: PASS" or "TWICET <mode>: FAIL". */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qdos386.h"

#define ERR_MAGIC (-21)
#define ERR_ARGS  (-34)
#define PG_ORIGIN (-61)
#define PG_REFUSED (-62)

static int fails;
static int na;

static void expect(const char *what, int ok)
{
	if (!ok) {
		fails++;
		printf("TWICET: CHECK FAILED %s\n", what);
	}
}

static int wait_exit(int id)
{
	int st;

	do {
		st = qd_job_exit_status(id);
		if (st == QD_ERR_RUNNING)
			qd_yield(1);
	} while (st == QD_ERR_RUNNING);
	return st;
}

static void wait_reclaimed(int id)
{
	while (!qd_job_reclaimed(id))
		qd_yield(1);
}

struct base {
	struct qd_pginfo pg;
	unsigned int heap;
};

static void snapshot(struct base *b)
{
	qd_pginfo(0, &b->pg);
	b->heap = qd_free_bytes();
}

static int same_as(const char *what, const struct base *b)
{
	struct base s;

	snapshot(&s);
	if (s.pg.spaces_live != b->pg.spaces_live || s.pg.held_bytes != b->pg.held_bytes ||
	    s.heap != b->heap) {
		printf("TWICET: %s: spaces %u/%u held %x/%x heap %u/%u\n", what, s.pg.spaces_live,
		       b->pg.spaces_live, s.pg.held_bytes, b->pg.held_bytes, s.heap, b->heap);
		fails++;
		return 0;
	}
	return 1;
}

static void hexs(char *dst, unsigned int v)
{
	int k;

	for (k = 28; k >= 0; k -= 4)
		*dst++ = "0123456789abcdef"[(v >> k) & 15u];
	*dst = 0;
}

static unsigned int hexv(const char *s)
{
	unsigned int v = 0;

	for (; *s; s++) {
		v <<= 4;
		v |= (*s >= 'a') ? (unsigned) (*s - 'a' + 10) : (unsigned) (*s - '0');
	}
	return v;
}

/* Two TWICE copies, tags 1 and 2, held then activated together.  Returns 1 if both exit 0x4t. */
static int pair(const char *ticks, int check_file, int *overlap_out)
{
	static char pk1[64], pk2[64];
	unsigned int l1, l2;
	int a, b, sa, sb, overlap = 0, polls = 0;
	struct qd_pginfo ia, ib;

	l1 = (unsigned int) sprintf(pk1, "%s", ticks) + 1;
	pk1[l1++] = '1';
	pk1[l1++] = 0;
	l2 = (unsigned int) sprintf(pk2, "%s", ticks) + 1;
	pk2[l2++] = '2';
	pk2[l2++] = 0;
	if (check_file) {
		l1 += (unsigned int) sprintf(pk1 + l1, "BIG.BIN") + 1;
		l1 += (unsigned int) sprintf(pk1 + l1, "c4e80000") + 1;
		l2 += (unsigned int) sprintf(pk2 + l2, "BIG.BIN") + 1;
		l2 += (unsigned int) sprintf(pk2 + l2, "c4e80000") + 1;
	}
	a = qd_pg_startv("TWICE.AOU", pk1, l1, QD_PG_HELD);
	b = qd_pg_startv("TWICE.AOU", pk2, l2, QD_PG_HELD);
	printf("TWICET: held starts %d and %d\n", a, b);
	if (a <= 0 || b <= 0) {
		if (a > 0) qd_pg_cancel(a);
		if (b > 0) qd_pg_cancel(b);
		expect("both started", 0);
		return 0;
	}
	qd_yield(5);
	qd_pginfo(a, &ia);
	qd_pginfo(b, &ib);
	expect("held copies are not dispatched", ia.dispatches == 0 && ib.dispatches == 0);
	expect("held copies still running", qd_job_exit_status(a) == QD_ERR_RUNNING &&
	       qd_job_exit_status(b) == QD_ERR_RUNNING);
	expect("distinct address spaces", ia.space != ib.space && ia.paged == 1 && ib.paged == 1);
	expect("both windows at 0x240000", ia.vbase == 0x240000u && ib.vbase == 0x240000u);
	expect("activate A", qd_job_activate(a, 2) == 0);
	expect("activate B", qd_job_activate(b, 2) == 0);
	while (!overlap && polls < 4000) {
		polls++;
		if (qd_pginfo(a, &ia) == 0 && qd_pginfo(b, &ib) == 0 && ia.dispatches > 0 && ib.dispatches > 0 &&
		    qd_job_exit_status(a) == QD_ERR_RUNNING && qd_job_exit_status(b) == QD_ERR_RUNNING)
			overlap = 1;
		else
			qd_yield(1);
	}
	if (overlap_out)
		*overlap_out = overlap;
	expect("live intervals overlap", overlap);
	sa = wait_exit(a);
	sb = wait_exit(b);
	printf("TWICET: exits %x %x\n", sa, sb);
	expect("copy 1 exit 0x41", sa == 0x41);
	expect("copy 2 exit 0x42", sb == 0x42);
	wait_reclaimed(a);
	wait_reclaimed(b);
	expect("stale id 1", qd_pginfo(a, &ia) == QD_ERR_NOJOB);
	expect("stale id 2", qd_pginfo(b, &ib) == QD_ERR_NOJOB);
	return sa == 0x41 && sb == 0x42;
}

static void mode_main(void)
{
	struct base b;
	int ov = 0, cyc;

	expect("paging available", qd_pg_available());
	printf("TWICET: caps %08x\n", (unsigned int) qd_pg_caps());
	snapshot(&b);
	pair("30", 0, &ov);
	same_as("after the pair", &b);
	pair("20", 1, &ov);              /* both read BIG.BIN (192 KiB) at the same time */
	same_as("after the file pair", &b);
	for (cyc = 0; cyc < 3; cyc++) {
		pair("10", 0, &ov);
		same_as("after a relaunch cycle", &b);
	}
}

static void mode_held(void)
{
	struct base b;
	int a, c, d, st;
	struct qd_pginfo in;

	snapshot(&b);
	a = qd_pg_start("TWICE.AOU", "5", QD_PG_HELD);
	expect("held start", a > 0);
	qd_pginfo(0, &in);
	expect("a space is held", in.spaces_live == b.pg.spaces_live + 1);
	expect("cancel a held job", qd_pg_cancel(a) == 0);
	same_as("cancel released everything", &b);
	expect("cancelled id is stale", qd_pg_cancel(a) == QD_ERR_NOJOB);
	expect("cancel unknown id", qd_pg_cancel(0x7ffe0005) == QD_ERR_NOJOB);
	expect("cancel self refused", qd_pg_cancel(qd_self()) == QD_PG_ERR_NOTHELD);

	/* A live (activated) copy cannot be cancelled. */
	{
		static const char pk[] = "20\0" "1";
		a = qd_pg_startv("TWICE.AOU", pk, 5, QD_PG_HELD);
	}
	expect("held start 2", a > 0);
	qd_job_activate(a, 2);
	expect("cancel of an active job refused", qd_pg_cancel(a) == QD_PG_ERR_NOTHELD);
	st = wait_exit(a);
	expect("the refused cancel left the job alone", st == 0x41);
	wait_reclaimed(a);
	same_as("after the live job", &b);

	/* Group rollback: three held copies (the record limit), a fourth refused, all cancelled. */
	a = qd_pg_start("TWICE.AOU", "5", QD_PG_HELD);
	c = qd_pg_start("TWICE.AOU", "5", QD_PG_HELD);
	d = qd_pg_start("TWICE.AOU", "5", QD_PG_HELD);
	printf("TWICET: group start %d %d %d\n", a, c, d);
	expect("first two started", a > 0 && c > 0);
	if (d <= 0)
		printf("TWICET: third refused with %d (limit of this machine)\n", d);
	{
		int e = qd_pg_start("TWICE.AOU", "5", QD_PG_HELD);
		printf("TWICET: next start -> %d\n", e);
		expect("beyond the limit is refused", e < 0);
	}
	if (a > 0) expect("cancel 1", qd_pg_cancel(a) == 0);
	if (c > 0) expect("cancel 2", qd_pg_cancel(c) == 0);
	if (d > 0) expect("cancel 3", qd_pg_cancel(d) == 0);
	same_as("group rollback left nothing", &b);
	a = qd_pg_start("TWICE.AOU", "5", 0);
	expect("a normal start after the rollback", a > 0);
	if (a > 0) {
		wait_exit(a);
		wait_reclaimed(a);
	}
	same_as("after the normal start", &b);
}

static void mode_neg(void)
{
	struct base b;
	int r, a, st;
	static const char noterm[2] = { '1', 'x' };

	snapshot(&b);
	r = qd_pg_start("README.TXT", 0, 0);
	printf("TWICET: bad header -> %d\n", r);
	expect("bad header", r == ERR_MAGIC);
	r = qd_pg_start("MULTIA.AOU", 0, 0);
	printf("TWICET: profile-2 image -> %d\n", r);
	expect("wrong origin", r == PG_ORIGIN);
	r = qd_pg_start("NOSUCH.AOU", 0, 0);
	printf("TWICET: missing file -> %d\n", r);
	expect("missing file", r < 0);
	r = qd_pg_startv("TWICE.AOU", noterm, 2, 0);
	printf("TWICET: unterminated args -> %d\n", r);
	expect("unterminated args", r == ERR_ARGS);
	r = qtrap(0x25, (unsigned int) "TWICE.AOU", 0, 0x00020000u);
	printf("TWICET: unknown flag -> %d\n", r);
	expect("unknown flag", r == ERR_ARGS);
	r = qd_pg_start((const char *) 0x01000000ul, 0, 0);
	expect("name outside memory", r < 0);
	r = qd_pginfo(0x7ffe0005, (struct qd_pginfo *) &b);
	expect("info of a never-created id", r == QD_ERR_NOJOB);
	same_as("failed starts left nothing", &b);

	/* A paged caller may not start programs or cancel: run the child. */
	{
		static const char pk[] = "C\0R";
		a = qd_pg_startv("TWICET.AOU", pk, 4, 0);
	}
	expect("child R started", a > 0);
	st = wait_exit(a);
	printf("TWICET: refusal child exit %x\n", st);
	expect("paged caller is refused", st == 0x70);
	wait_reclaimed(a);
	same_as("after the child", &b);
}

static void mode_exhaust(void)
{
	struct base b;
	int ids[8], n = 0, r, i;

	snapshot(&b);
	for (;;) {
		r = qd_pg_start("TWICE.AOU", "5", QD_PG_HELD);
		if (r <= 0 || n >= 6)
			break;
		ids[n++] = r;
	}
	printf("TWICET: %d held starts succeeded; the next returned %d\n", n, r);
	expect("the machine refuses beyond its limit", r < 0);
	expect("at least two copies fit", n >= 2);
	expect("refusal is a bounded paging/job code", r == -60 || r == -63 || r == -29 || r == -30);
	for (i = 0; i < n; i++)
		expect("cancel", qd_pg_cancel(ids[i]) == 0);
	same_as("exhaustion rolled back", &b);
	r = qd_pg_start("TWICE.AOU", "5", 0);
	expect("recovered", r > 0);
	if (r > 0) {
		wait_exit(r);
		wait_reclaimed(r);
	}
	same_as("after recovery", &b);
}

static void mode_cycles(void)
{
	struct base b;
	int cyc, a, c;
	unsigned int s1 = 0, s2 = 0;
	struct qd_pginfo ia, ib;
	static const char p1[] = "6\0" "1", p2[] = "6\0" "2";

	snapshot(&b);
	for (cyc = 0; cyc < 10; cyc++) {
		a = qd_pg_startv("TWICE.AOU", p1, 4, QD_PG_HELD);
		c = qd_pg_startv("TWICE.AOU", p2, 4, QD_PG_HELD);
		if (a <= 0 || c <= 0) {
			printf("TWICET: cycle %d start %d %d\n", cyc, a, c);
			fails++;
			break;
		}
		qd_pginfo(a, &ia);
		qd_pginfo(c, &ib);
		if (cyc == 0) { s1 = ia.space; s2 = ib.space; }
		expect("space reused", ia.space == s1 && ib.space == s2);
		qd_job_activate(a, 2);
		qd_job_activate(c, 2);
		expect("cycle exit 1", wait_exit(a) == 0x41);
		expect("cycle exit 2", wait_exit(c) == 0x42);
		wait_reclaimed(a);
		wait_reclaimed(c);
		same_as("cycle released everything", &b);
	}
	printf("TWICET: %d cycles\n", cyc);
}

static void mode_fault_launch(void)
{
	struct base b;
	int r;

	snapshot(&b);
	r = qd_pg_start("TWICE.AOU", "5", 0);
	printf("TWICET: launch with injected read failure -> %d\n", r);
	expect("injected failure reported", r == -40);
	same_as("failed copy released everything", &b);
	pair("10", 0, 0);
	same_as("after the pair", &b);
}

static void mode_fault(void)
{
	struct base b;
	int h, f1, f2, f3, un, s;
	static const char ph[] = "40\0" "1", pf[] = "C\0F", pd[] = "C\0D", pu[] = "C\0U";

	snapshot(&b);
	h = qd_pg_startv("TWICE.AOU", ph, 5, QD_PG_HELD);
	f1 = qd_pg_startv("TWICET.AOU", pf, 4, QD_PG_HELD);
	expect("started", h > 0 && f1 > 0);
	if (h <= 0 || f1 <= 0)
		return;
	qd_job_activate(h, 2);
	qd_job_activate(f1, 2);
	s = wait_exit(f1);
	printf("TWICET: page-fault copy ended with %d\n", s);
	expect("page-fault copy ended by the fault", s == QD_ERR_NOSTATUS);
	wait_reclaimed(f1);
	f2 = qd_pg_startv("TWICET.AOU", pd, 4, 0);
	expect("divide copy started", f2 > 0);
	s = wait_exit(f2);
	printf("TWICET: divide-error copy ended with %d\n", s);
	expect("divide-error copy ended", s == QD_ERR_NOSTATUS);
	wait_reclaimed(f2);
	f3 = qd_pg_startv("TWICET.AOU", pu, 4, 0);
	expect("opcode copy started", f3 > 0);
	s = wait_exit(f3);
	printf("TWICET: invalid-opcode copy ended with %d\n", s);
	expect("invalid-opcode copy ended", s == QD_ERR_NOSTATUS);
	wait_reclaimed(f3);
	s = wait_exit(h);
	printf("TWICET: healthy copy beside three faults exit %x\n", s);
	expect("healthy copy unaffected", s == 0x41);
	wait_reclaimed(h);
	/* An unpaged job beside a fault: this launcher itself is one and is still here. */
	un = qd_ticks() > 0;
	expect("unpaged launcher alive", un);
	same_as("faulted copies released", &b);
	pair("10", 0, 0);
	same_as("after a normal pair", &b);
}

static void mode_console(void)
{
	struct base b;
	static const char pw[] = "C\0W";
	int a, c, sa, sc;

	snapshot(&b);
	a = qd_pg_startv("TWICET.AOU", pw, 4, 0);
	qd_yield(10);
	if (a > 0 && qd_job_exit_status(a) != QD_ERR_RUNNING) {
		/* qd_con_wait returns at once when input or end-of-input is already pending (a piped
		 * stdin): there is nothing to serialise, so the test cannot run on this harness. */
		printf("TWICET: the first waiter returned at once (input or EOF already pending): NA here\n");
		na = 1;
		wait_reclaimed(a);
		return;
	}
	c = qd_pg_startv("TWICET.AOU", pw, 4, 0);
	expect("both started", a > 0 && c > 0);
	sc = wait_exit(c);
	printf("TWICET: second console waiter exit %x\n", sc);
	expect("second waiter refused with -50", sc == 0x71);
	expect("first waiter still waiting", qd_job_exit_status(a) == QD_ERR_RUNNING);
	qd_job_reactivate(a);
	sa = wait_exit(a);
	printf("TWICET: first console waiter exit %x\n", sa);
	expect("first waiter released", sa == 0x70);
	wait_reclaimed(a);
	wait_reclaimed(c);
	same_as("after the console test", &b);
}

static void mode_shared(void)
{
	struct base b;
	unsigned long *blk = (unsigned long *) qd_alloc(16);
	char pk[40];
	unsigned int l;
	int a, c;

	snapshot(&b);
	blk[0] = 0;
	l = (unsigned int) sprintf(pk, "C") + 1;
	l += (unsigned int) sprintf(pk + l, "Z") + 1;
	hexs(pk + l, (unsigned int) blk);
	l += 9;
	a = qd_pg_startv("TWICET.AOU", pk, l, 0);
	expect("writer started", a > 0);
	expect("writer exit", wait_exit(a) == 0x70);
	printf("TWICET: kernel-heap block %08x holds %08x after copy A\n", (unsigned int) blk, (unsigned int) blk[0]);
	expect("copy A's write is visible to the launcher", blk[0] == 0x5A5A5A5Aul);
	pk[l - 10] = 'z';              /* the reader mode */
	c = qd_pg_startv("TWICET.AOU", pk, l, 0);
	expect("reader started", c > 0);
	expect("copy B read copy A's value (heap is shared, not isolated)", wait_exit(c) == 0x70);
	wait_reclaimed(a);
	wait_reclaimed(c);
	qd_free(blk);
	{
		struct base s;
		snapshot(&s);
		expect("heap restored", s.heap == b.heap + 0 || s.heap >= b.heap);
	}
}

static void mode_tasks(void)
{
	int before[6], mid[6], after[6], i, a, c, st;
	static const char p1[] = "40\0" "1", p2[] = "40\0" "2";
	struct base b;

	snapshot(&b);
	for (i = 0; i < 6; i++)
		before[i] = qtrap(0x28, (unsigned int) i, 0, 0);
	printf("TWICET: kernel tasks before: poll %d (paged %d, off-template %d)  sched %d (paged %d, off-template %d)\n",
	       before[0], before[1], before[2], before[3], before[4], before[5]);
	expect("test tasks present", before[0] >= 0 && before[3] >= 0);
	a = qd_pg_startv("TWICE.AOU", p1, 5, QD_PG_HELD);
	c = qd_pg_startv("TWICE.AOU", p2, 5, QD_PG_HELD);
	expect("started", a > 0 && c > 0);
	qd_job_activate(a, 2);
	qd_job_activate(c, 2);
	qd_yield(20);
	for (i = 0; i < 6; i++)
		mid[i] = qtrap(0x28, (unsigned int) i, 0, 0);
	st = wait_exit(a);
	expect("copy 1", st == 0x41);
	st = wait_exit(c);
	expect("copy 2", st == 0x42);
	wait_reclaimed(a);
	wait_reclaimed(c);
	for (i = 0; i < 6; i++)
		after[i] = qtrap(0x28, (unsigned int) i, 0, 0);
	printf("TWICET: kernel tasks after: poll %d (paged %d, off-template %d)  sched %d (paged %d, off-template %d)\n",
	       after[0], after[1], after[2], after[3], after[4], after[5]);
	expect("poll tasks ran", after[0] > before[0]);
	expect("sched tasks ran", after[3] > before[3]);
	expect("poll tasks ran while a paged job was current", after[1] > before[1]);
	expect("sched tasks ran while a paged job was current", after[4] > before[4]);
	expect("no task ever ran under a paged directory", after[2] == 0 && after[5] == 0);
	same_as("after the task test", &b);
	(void) mid;
}

static void mode_nopaging(void)
{
	struct qd_pginfo pi;

	if (qd_pg_available()) {
		printf("TWICET X: this kernel has paging\n");
		na = 1;
		return;
	}
	expect("qd_pg_available is 0", qd_pg_available() == 0);
	expect("qd_pg_caps", qd_pg_caps() == QD_PG_ERR_NOPAGING);
	expect("qd_pg_start", qd_pg_start("TWICE.AOU", 0, 0) == QD_PG_ERR_NOPAGING);
	expect("qd_pg_start held", qd_pg_start("TWICE.AOU", "1", QD_PG_HELD) == QD_PG_ERR_NOPAGING);
	expect("qd_pg_cancel", qd_pg_cancel(qd_self()) == QD_PG_ERR_NOPAGING);
	expect("qd_pginfo", qd_pginfo(0, &pi) == QD_PG_ERR_NOPAGING);
	printf("TWICET X: paging calls return %d on this kernel\n", QD_PG_ERR_NOPAGING);
}

/* ---- review regression Y ---- */
static struct { void *next; void (*fn)(void); } ynode;
static volatile int yarmed;

static void ytask(void)
{
	if (yarmed)
		*(volatile unsigned long *) 0x01000000ul = 1;      /* faults in task context */
}

static void mode_orphan(void)
{
	static const char pk[] = "300\0" "1";
	int a = qd_pg_startv("TWICE.AOU", pk, 6, QD_PG_HELD);

	expect("paged copy started", a > 0);
	if (a <= 0)
		return;
	ynode.fn = ytask;
	qtrap(0x0E, (unsigned int) &ynode, 0, 0);
	qd_job_activate(a, 2);
	qd_yield(5);                    /* the copy is dispatched and is current at the next ticks */
	printf("TWICET Y: arming a faulting unpaged poll task at %08x (window code)\n", (unsigned int) ytask);
	yarmed = 1;
	qd_yield(200);
	printf("TWICET Y: the machine did NOT halt\n");
	fails++;
}

/* ---- paged child ---- */
static int child(const char *sub, const char *arg)
{
	struct qd_pginfo pi;

	switch (sub[0]) {
	case 'R': {
		int ok = 1, r;
		r = qd_pg_start("TWICE.AOU", 0, 0);
		if (r != PG_REFUSED) { printf("child: pg_start -> %d\n", r); ok = 0; }
		r = qd_run("TWICE.AOU", 0);
		if (r != PG_REFUSED) { printf("child: run -> %d\n", r); ok = 0; }
		r = qd_start("TWICE.AOU", 1);
		if (r != PG_REFUSED) { printf("child: start -> %d\n", r); ok = 0; }
		r = qd_pg_cancel(qd_self());
		if (r != PG_REFUSED) { printf("child: cancel -> %d\n", r); ok = 0; }
		if (!qd_pg_available()) { printf("child: caps unavailable\n"); ok = 0; }
		if (qd_pginfo(0, &pi) != 0 || pi.paged != 1) { printf("child: not paged\n"); ok = 0; }
		printf("child R: refusals %s, own space %05x\n", ok ? "all -62" : "WRONG", pi.space);
		return ok ? 0x70 : 0x7f;
	}
	case 'F':
		printf("child F: touching 01000000\n");
		return (int) *(volatile unsigned long *) 0x01000000ul;
	case 'D': {
		volatile int z = 0;
		printf("child D: dividing by zero\n");
		return 100 / z;
	}
	case 'U':
		printf("child U: invalid opcode\n");
		asm volatile (".byte 0x0f,0x0b");
		return 0x7e;
	case 'W': {
		static volatile unsigned char flag;
		int rc;
		flag = 0;
		rc = qd_con_wait((unsigned char *) &flag);
		printf("child W: qd_con_wait -> %d\n", rc);
		return rc == 0 ? 0x70 : (rc == QD_ERR_CONBUSY ? 0x71 : 0x7f);
	}
	case 'Z': case 'z': {
		unsigned long *blk = (unsigned long *) hexv(arg);
		if (sub[0] == 'Z') {
			*blk = 0x5A5A5A5Aul;
			return 0x70;
		}
		return *blk == 0x5A5A5A5Aul ? 0x70 : 0x7f;
	}
	}
	return 0x7f;
}

int main(int argc, char **argv)
{
	int mode = argc >= 2 ? argv[1][0] : 'M';

	if (mode == 'C')
		return child(argc >= 3 ? argv[2] : "?", argc >= 4 ? argv[3] : "");
	printf("TWICET %c: start\n", mode);
	if (mode == 'X') {
		mode_nopaging();
		goto verdict;
	}
	if (!qd_pg_available() && mode != 'N') {
		printf("TWICET %c: this kernel has no paging\n", mode);
		printf("TWICET %c: NA\n", mode);
		return 3;
	}
	switch (mode) {
	case 'H': mode_held(); break;
	case 'N': mode_neg(); break;
	case 'E': mode_exhaust(); break;
	case 'S': mode_cycles(); break;
	case 'T': mode_fault_launch(); break;
	case 'F': mode_fault(); break;
	case 'W': mode_console(); break;
	case 'Z': mode_shared(); break;
	case 'K': mode_tasks(); break;
	case 'Y': mode_orphan(); break;
	default: mode_main(); break;
	}
verdict:
	if (na && fails == 0) {
		printf("TWICET %c: NA\n", mode);
		return 0;
	}
	printf("TWICET %c: %s\n", mode, fails == 0 ? "PASS" : "FAIL");
	return fails == 0 ? 0 : 1;
}
