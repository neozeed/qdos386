/* R11J SAMELNCH (the plan's SAME-LAUNCH; FAT 8.3 name): an ordinary slot-1 program, run by
 * the shell with RUN, so it is itself unpaged and lives at physical 0x240000 under the
 * kernel template.  It starts copies of the one file SAME.AOU through QT_PG_START; each copy
 * gets its own directory and frames but the same virtual origin 0x240000.
 *
 *   SAMELNCH        main proof: two live copies, distinct directories and frames, overlap,
 *                   a third launch while both live (bounded refusal), exits, reclamation
 *   SAMELNCH N      negative cases: bad header, wrong origin, missing file, bad arguments,
 *                   bad pointers, stale ids, failed launches leave nothing behind
 *   SAMELNCH R      repeated launch/exit cycles with frame reuse and no leak
 *   SAMELNCH T      first paged launch expected to fail in the copy (test kernel built with
 *                   -DQDOS_TEST_FAULT_LAUNCH); checks rollback, then a normal pair
 *   SAMELNCH F      page faults: a copy touching another copy's frames, and a copy touching
 *                   an unmapped address; the faulting copies end, the others continue
 * Prints "SAMELNCH <mode>: PASS" or "SAMELNCH <mode>: FAIL". */
#include <stdio.h>
#include "qdos386.h"

#define QT_PG_START 0x25
#define QT_PG_INFO  0x26

#define PG_VBASE 0x00240000u
#define ERR_IO        (-40)
#define ERR_MAGIC     (-21)
#define ERR_ARGS      (-34)
#define PG_ERR_ORIGIN (-61)

struct pg_info {
	unsigned int paged, cr3, frame_base, frame_bytes, vbase, vsize, held_bytes, cr3_loads;
	unsigned int preempts, dispatches, region_count, region_bytes, pte_vbase, cur_cr3;
	unsigned int holds_deferred, records_live, int27_count, pool_lo;
};

static int fails;

static int pg_start(const char *name, const char *args, unsigned int alen)
{
	return qtrap(QT_PG_START, (unsigned int) name, (unsigned int) args, alen);
}

static int pg_info(int id, struct pg_info *in)
{
	return qtrap(QT_PG_INFO, (unsigned int) id, (unsigned int) in, 0);
}

static void expect(const char *what, int ok)
{
	if (!ok) {
		fails++;
		printf("SAMELNCH: CHECK FAILED %s\n", what);
	}
}

static int wait_exit(int id)
{
	int st;

	do {
		st = qd_job_exit_status(id);
		if (st == QD_ERR_RUNNING)
			qd_yield(2);
	} while (st == QD_ERR_RUNNING);
	return st;
}

static void wait_reclaimed(int id)
{
	while (!qd_job_reclaimed(id))
		qd_yield(1);
}

static void show(const char *tag, int id, struct pg_info *in)
{
	printf("SAMELNCH %s: id %d paged %u cr3 %08x frames %08x+%x pte %08x vbase %08x held %x regions %u/%x\n",
	       tag, id, in->paged, in->cr3, in->frame_base, in->frame_bytes, in->pte_vbase,
	       in->vbase, in->held_bytes, in->region_count, in->region_bytes);
}

/* Baseline: held bytes and application-pool regions with no paged process. */
static void baseline(struct pg_info *b)
{
	expect("self info", pg_info(0, b) == 0);
	show("self", qd_self(), b);
	expect("launcher unpaged", b->paged == 0);
}

static void same_state(const char *what, struct pg_info *b)
{
	struct pg_info s;

	pg_info(0, &s);
	expect(what, s.held_bytes == b->held_bytes && s.region_count == b->region_count &&
	             s.region_bytes == b->region_bytes && s.records_live == b->records_live);
}

static void mode_main(void)
{
	struct pg_info b, ia, ib, s;
	int a, c, sa, sb, overlap = 0, polls = 0;
	unsigned long t;

	baseline(&b);
	a = pg_start("SAME.AOU", "1", 2);
	c = pg_start("SAME.AOU", "2", 2);
	printf("SAMELNCH: started %d and %d from one file\n", a, c);
	expect("both started", a > 0 && c > 0);
	if (a <= 0 || c <= 0)
		return;
	expect("info A", pg_info(a, &ia) == 0);
	expect("info B", pg_info(c, &ib) == 0);
	show("A", a, &ia);
	show("B", c, &ib);
	expect("both paged", ia.paged == 1 && ib.paged == 1);
	expect("same virtual base", ia.vbase == PG_VBASE && ib.vbase == PG_VBASE);
	expect("different directories", ia.cr3 != ib.cr3);
	expect("neither is the launcher's", ia.cr3 != b.cr3 && ib.cr3 != b.cr3);
	expect("different frames", ia.frame_base != ib.frame_base);
	expect("frames do not overlap", ia.frame_base + ia.frame_bytes <= ib.frame_base ||
	                                ib.frame_base + ib.frame_bytes <= ia.frame_base);
	expect("pte maps own frames", (ia.pte_vbase >> 12) == (ia.frame_base >> 12) &&
	                              (ib.pte_vbase >> 12) == (ib.frame_base >> 12));
	expect("launcher maps 0x240000 to itself", (b.pte_vbase >> 12) == (PG_VBASE >> 12));

	/* A third copy while both live: refused with a bounded error, nothing left behind. */
	pg_info(0, &s);
	{
		int x = pg_start("SAME.AOU", "3", 2);
		struct pg_info s2;
		printf("SAMELNCH: third launch -> %d (held %x)\n", x, s.held_bytes);
		expect("third launch refused", x < 0);
		pg_info(0, &s2);
		expect("third launch left nothing", s2.held_bytes == s.held_bytes &&
		       s2.region_count == s.region_count && s2.records_live == s.records_live);
	}

	/* An unpaged job's hold flag in its own slot-1 memory (0x24xxxx), released by the timer
	 * while a paged copy may be running: the kernel must write it in the launcher's (template)
	 * directory, not into the paged copy's window (R11J review defect 1). */
	{
		static volatile unsigned char lflag;
		int k, ok = 0;
		for (k = 0; k < 8; k++) {
			lflag = 0;
			qtrap(0x08, 2, (unsigned int) &lflag, 0);
			if (lflag == 0xff)
				ok++;
		}
		printf("SAMELNCH: unpaged hold flag at %08x set %d of 8\n", (unsigned int) &lflag, ok);
		expect("unpaged hold flag set in its own space", ok == 8);
	}

	/* Overlap: both have been dispatched and neither has ended, at the same instant. */
	while (!overlap && polls < 2000) {
		polls++;
		if (pg_info(a, &ia) == 0 && pg_info(c, &ib) == 0 &&
		    ia.dispatches > 0 && ib.dispatches > 0 &&
		    qd_job_exit_status(a) == QD_ERR_RUNNING && qd_job_exit_status(c) == QD_ERR_RUNNING) {
			overlap = 1;
			t = qd_ticks();
			printf("SAMELNCH: overlap at tick %u: A disp %u preempts %u, B disp %u preempts %u\n",
			       (unsigned int) t, ia.dispatches, ia.preempts, ib.dispatches, ib.preempts);
		} else {
			qd_yield(1);
		}
	}
	expect("live intervals overlap", overlap);

	sa = wait_exit(a);
	sb = wait_exit(c);
	printf("SAMELNCH: exits %x %x\n", sa, sb);
	expect("A exit 0x51", sa == 0x51);
	expect("B exit 0x52", sb == 0x52);
	wait_reclaimed(a);
	wait_reclaimed(c);
	pg_info(0, &s);
	show("after", qd_self(), &s);
	same_state("frames and records released", &b);
	expect("stale A refused", pg_info(a, &ia) == QD_ERR_NOJOB);
	expect("stale B refused", pg_info(c, &ib) == QD_ERR_NOJOB);
	printf("SAMELNCH: cr3 loads %u, hold writes deferred %u, irq7 entries %u\n", s.cr3_loads,
	       s.holds_deferred, s.int27_count);
}

static void mode_neg(void)
{
	struct pg_info b, s;
	int r, q;
	static char noterm[2] = { '1', 'x' };

	baseline(&b);
	r = pg_start("README.TXT", 0, 0);
	printf("SAMELNCH: bad header -> %d\n", r);
	expect("bad header", r == ERR_MAGIC);
	same_state("bad header left nothing", &b);
	r = pg_start("MULTIA.AOU", 0, 0);
	printf("SAMELNCH: profile-2 image -> %d\n", r);
	expect("wrong origin", r == PG_ERR_ORIGIN);
	same_state("wrong origin left nothing", &b);
	r = pg_start("NOSUCH.AOU", 0, 0);
	printf("SAMELNCH: missing file -> %d\n", r);
	expect("missing file", r < 0);
	same_state("missing file left nothing", &b);
	r = pg_start("SAME.AOU", noterm, 2);
	printf("SAMELNCH: unterminated arguments -> %d\n", r);
	expect("bad arguments", r == ERR_ARGS);
	same_state("bad arguments left nothing", &b);
	r = pg_start((const char *) 0x01000000ul, 0, 0);
	printf("SAMELNCH: name outside memory -> %d\n", r);
	expect("bad name pointer", r < 0);
	r = pg_info(0, (struct pg_info *) 0x01000000ul);
	printf("SAMELNCH: info into unmapped buffer -> %d\n", r);
	expect("bad info buffer", r < 0);
	r = pg_info(0x7ffe0005, &s);
	printf("SAMELNCH: info for never-created id -> %d\n", r);
	expect("bad id", r == QD_ERR_NOJOB);

	q = pg_start("SAME.AOU", "4\0Q", 4);
	expect("quick copy started", q > 0);
	r = wait_exit(q);
	printf("SAMELNCH: quick copy %d exit %x\n", q, r);
	expect("quick exit 0x54", r == 0x54);
	wait_reclaimed(q);
	r = pg_info(q, &s);
	printf("SAMELNCH: stale id %d -> %d\n", q, r);
	expect("stale id", r == QD_ERR_NOJOB);
	same_state("quick copy released", &b);
}

static void mode_repeat(void)
{
	struct pg_info b, ia, ib;
	int cycle, a, c, sa, sb;
	unsigned int fa0 = 0, fb0 = 0;

	baseline(&b);
	for (cycle = 0; cycle < 8; cycle++) {
		a = pg_start("SAME.AOU", "1\0W", 4);
		c = pg_start("SAME.AOU", "2\0W", 4);
		if (a <= 0 || c <= 0) {
			printf("SAMELNCH: cycle %d launch %d %d\n", cycle, a, c);
			fails++;
			break;
		}
		/* Both copies stay live for 20 ticks ("W"), so their records can be read. */
		if (pg_info(a, &ia) != 0 || pg_info(c, &ib) != 0) {
			printf("SAMELNCH: cycle %d info refused\n", cycle);
			fails++;
			break;
		}
		expect("cycle copies paged", ia.paged == 1 && ib.paged == 1 && ia.cr3 != ib.cr3);
		if (cycle == 0) {
			fa0 = ia.frame_base;
			fb0 = ib.frame_base;
		}
		sa = wait_exit(a);
		sb = wait_exit(c);
		wait_reclaimed(a);
		wait_reclaimed(c);
		printf("SAMELNCH: cycle %d ids %d %d frames %08x %08x cr3 %08x %08x exits %x %x\n",
		       cycle, a, c, ia.frame_base, ib.frame_base, ia.cr3, ib.cr3, sa, sb);
		expect("cycle exits", sa == 0x51 && sb == 0x52);
		expect("frames reused", ia.frame_base == fa0 && ib.frame_base == fb0);
		same_state("cycle released everything", &b);
	}
}

static void mode_fault_launch(void)
{
	struct pg_info b;
	int r, a, c;

	baseline(&b);
	r = pg_start("SAME.AOU", "1\0Q", 4);
	printf("SAMELNCH: launch with injected read failure -> %d\n", r);
	expect("injected failure reported", r == ERR_IO);
	same_state("failed copy released everything", &b);
	a = pg_start("SAME.AOU", "1\0Q", 4);
	c = pg_start("SAME.AOU", "2\0Q", 4);
	expect("pair after failure", a > 0 && c > 0);
	if (a > 0 && c > 0) {
		expect("A exit", wait_exit(a) == 0x51);
		expect("B exit", wait_exit(c) == 0x52);
		wait_reclaimed(a);
		wait_reclaimed(c);
	}
	same_state("after the pair", &b);
}

static void mode_fault(void)
{
	struct pg_info b, ia;
	int a, p, f, sa, sp, sf;
	static char args[32];
	char *q;
	unsigned int v;
	int k;

	baseline(&b);
	a = pg_start("SAME.AOU", "1", 2);
	expect("A started", a > 0 && pg_info(a, &ia) == 0);
	if (a <= 0)
		return;
	/* args = "2\0P\0<hex frames of A>\0" */
	q = args;
	*q++ = '2'; *q++ = 0; *q++ = 'P'; *q++ = 0;
	v = ia.frame_base;
	for (k = 28; k >= 0; k -= 4)
		*q++ = "0123456789abcdef"[(v >> k) & 15u];
	*q++ = 0;
	p = pg_start("SAME.AOU", args, (unsigned int) (q - args));
	expect("P started", p > 0);
	sp = wait_exit(p);
	printf("SAMELNCH: copy touching A's frames %08x ended with %d\n", ia.frame_base, sp);
	expect("P ended by the fault", sp == QD_ERR_NOSTATUS);
	f = pg_start("SAME.AOU", "3\0F", 4);
	expect("F started", f > 0);
	sf = wait_exit(f);
	printf("SAMELNCH: copy touching 01000000 ended with %d\n", sf);
	expect("F ended by the fault", sf == QD_ERR_NOSTATUS);
	sa = wait_exit(a);
	printf("SAMELNCH: A, running beside both faults, exit %x\n", sa);
	expect("A unaffected", sa == 0x51);
	wait_reclaimed(a);
	wait_reclaimed(p);
	wait_reclaimed(f);
	same_state("faulted copies released", &b);
}

int main(int argc, char **argv)
{
	int mode = argc >= 2 ? argv[1][0] : 'M';

	printf("SAMELNCH %c: start, launcher main %08x\n", mode, (unsigned int) main);
	if (mode == 'N')
		mode_neg();
	else if (mode == 'R')
		mode_repeat();
	else if (mode == 'T')
		mode_fault_launch();
	else if (mode == 'F')
		mode_fault();
	else
		mode_main();
	printf("SAMELNCH %c: %s\n", mode, fails == 0 ? "PASS" : "FAIL");
	return fails == 0 ? 0 : 1;
}
