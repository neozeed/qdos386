/* R11J SAME: one fixed-origin a.out (slot 1, entry 0x240000) that is started twice by
 * SAMELNCH through QT_PG_START, each copy in its own address space.  Built with the
 * unchanged SDK (qdos386-gcc -slot 1 -O); both copies are the same file bytes.
 *
 *   argv[1]  instance id, one digit (1..9)
 *   argv[2]  mode: absent = full run; "Q" = quick (state check only); "F" = touch the
 *            unmapped address 0x01000000 (deliberate page fault); "W" = like Q, but stays
 *            live for 20 ticks first (so a launcher can inspect it); "P" = touch argv[3]
 *            (hex physical address, another instance's frames), which this process's
 *            directory does not map.
 *
 * Full run: every instance writes its own pattern into the same virtual globals (BSS and
 * data), keeps a stack canary and an x87 accumulator, then runs three phases: CPU-bound
 * (switched only by the timer), sleeps with a hold flag in its own window (the kernel's
 * release write is made in this address space), and immediate yields (a CR3 switch per
 * yield while both copies run).  The state is checked after every step.  The exit status
 * is 0x50 + id when everything held, 0x5F otherwise. */
#include <stdio.h>
#include "qdos386.h"

#define QT_SUSPEND_RAW 0x08
#define QT_PG_INFO     0x26
#define ERR_RANGE      (-14)

#define NG      256
#define NCAN    32
#define P1_TICKS 48u
#define P2_SLEEPS 12
#define P3_YIELDS 400

struct pg_info {
	unsigned int paged, cr3, frame_base, frame_bytes, vbase, vsize, held_bytes, cr3_loads;
	unsigned int preempts, dispatches, region_count, region_bytes, pte_vbase, cur_cr3;
	unsigned int holds_deferred, records_live, int27_count, pool_lo;
};

static volatile unsigned long gvals[NG];          /* BSS: same virtual address in every copy */
static volatile unsigned long gdata = 0x5A5A0000ul;   /* data */
static volatile unsigned char hold_flag;
static int id;                 /* in the window: used for messages only */
static unsigned long bad;
static unsigned long checks;

/* The expected values are computed from sid, the tagged job id the kernel reports (qd_self),
 * kept on the job stack (kernel heap, outside the window).  Nothing in the window feeds the
 * expectation: if two copies shared frames (even their argument block), each would find the
 * other's pattern in gvals/gdata and count it as bad. */
static unsigned long pat(int sid, int i)
{
	return ((unsigned long) sid * 40503ul) ^ ((unsigned long) i * 2654435761ul);
}

static void check(int sid, volatile unsigned long *can)
{
	int i;

	checks++;
	for (i = 0; i < NG; i++)
		if (gvals[i] != pat(sid, i)) {
			bad++;
			return;
		}
	if (gdata != (0x5A5A0000ul ^ (unsigned long) sid))
		bad++;
	for (i = 0; i < NCAN; i++)
		if (can[i] != (0xCA000000ul ^ ((unsigned long) sid << 4) ^ (unsigned long) i))
			bad++;
}

static unsigned long hex_arg(const char *s)
{
	unsigned long v = 0;

	while (*s) {
		char c = *s++;
		v <<= 4;
		if (c >= '0' && c <= '9')
			v |= (unsigned long) (c - '0');
		else if (c >= 'a' && c <= 'f')
			v |= (unsigned long) (c - 'a' + 10);
		else if (c >= 'A' && c <= 'F')
			v |= (unsigned long) (c - 'A' + 10);
	}
	return v;
}

static unsigned int info_line(const char *tag)
{
	struct pg_info in;
	int rc = qtrap(QT_PG_INFO, 0, (unsigned int) &in, 0);

	printf("SAME %d %s: rc %d paged %u cr3 %08x frames %08x pte %08x cur %08x preempts %u disp %u deferred %u\n",
	       id, tag, rc, in.paged, in.cr3, in.frame_base, in.pte_vbase, in.cur_cr3,
	       in.preempts, in.dispatches, in.holds_deferred);
	return in.pool_lo;
}

/* Pointer checks in the kernel: own window and own heap block accepted, everything this
 * process does not map, or may not name, refused.  1-byte writes to the console. */
static void range_checks(unsigned int pool)
{
	int ch = qd_open("CON");
	char *blk = (char *) qd_alloc(16);
	static char mark[2] = ".";
	int r_own, r_heap, r_hi, r_pool, r_shell;

	if (blk != 0)
		blk[0] = ':';
	r_own = qd_write(ch, mark, 1);
	r_heap = blk ? qd_write(ch, blk, 1) : -99;
	r_hi = qd_write(ch, (const void *) 0x01000000ul, 1);
	r_pool = qd_write(ch, (const void *) pool, 1);
	r_shell = qd_write(ch, (const void *) 0x00200000ul, 1);
	printf("\nSAME %d ranges: own %d heap %d unmapped %d pool(%x) %d shell %d\n",
	       id, r_own, r_heap, r_hi, pool, r_pool, r_shell);
	if (r_own != 1 || r_heap != 1 || r_hi != ERR_RANGE || r_pool != ERR_RANGE ||
	    r_shell != ERR_RANGE)
		bad++;
	if (blk)
		qd_free(blk);
	qd_close(ch);
}

int main(int argc, char **argv)
{
	volatile unsigned long can[NCAN];
	unsigned long t0, t, last, n = 0, acc = 1;
	double x = 0.0, step;
	int i, k, mode = 0, sid;
	unsigned int pool;

	if (argc < 2 || argv[1][0] < '1' || argv[1][0] > '9') {
		printf("SAME: usage SAME id [Q|F|P addr]\n");
		return 0x5F;
	}
	id = argv[1][0] - '0';
	sid = qd_self();
	if (argc >= 3)
		mode = argv[2][0];
	step = 0.5 * (double) id;

	for (i = 0; i < NG; i++)
		gvals[i] = pat(sid, i);
	gdata = 0x5A5A0000ul ^ (unsigned long) sid;
	for (i = 0; i < NCAN; i++)
		can[i] = 0xCA000000ul ^ ((unsigned long) sid << 4) ^ (unsigned long) i;
	printf("SAME %d start: main %08x gvals %08x gdata %08x stack %08x tick %u\n", id,
	       (unsigned int) main, (unsigned int) gvals, (unsigned int) &gdata,
	       (unsigned int) can, (unsigned int) qd_ticks());
	pool = info_line("info");
	check(sid, can);

	if (mode == 'F') {
		printf("SAME %d: touching unmapped 01000000\n", id);
		i = (int) *(volatile unsigned long *) 0x01000000ul;
		printf("SAME %d: FAULT NOT RAISED %d\n", id, i);
		return 0x5E;
	}
	if (mode == 'P' && argc >= 4) {
		unsigned long a = hex_arg(argv[3]);
		printf("SAME %d: touching another process's frames at %08x\n", id, (unsigned int) a);
		i = (int) *(volatile unsigned long *) a;
		printf("SAME %d: FAULT NOT RAISED %d\n", id, i);
		return 0x5E;
	}
	if (mode == 'W')
		qd_yield(20);
	if (mode == 'Q' || mode == 'W') {
		check(sid, can);
		printf("SAME %d quick: checks %u bad %u\n", id, (unsigned int) checks, (unsigned int) bad);
		return bad == 0 ? 0x50 + id : 0x5F;
	}

	range_checks(pool);

	/* Phase 1: CPU-bound.  No service trap inside the inner loop: only the timer switches. */
	t0 = qd_ticks();
	last = t0;
	do {
		for (k = 0; k < 2000; k++) {
			acc = acc * 1664525ul + 1013904223ul;
			x += step;
			n++;
		}
		check(sid, can);
		t = qd_ticks();
		if (t - last >= 8u) {
			printf("SAME %d p1 tick %u n %u\n", id, (unsigned int) t, (unsigned int) n);
			last = t;
		}
	} while (t - t0 < P1_TICKS);
	if (x != step * (double) n) {
		printf("SAME %d: x87 accumulator wrong\n", id);
		bad++;
	}

	/* Phase 2: sleep with a hold flag in this window.  The kernel sets it to 0xFF when the
	 * timeout expires, possibly while another directory is active (deferred write). */
	for (i = 0; i < P2_SLEEPS; i++) {
		hold_flag = 0;
		qtrap(QT_SUSPEND_RAW, 2, (unsigned int) &hold_flag, 0);
		if (hold_flag != 0xff)
			bad++;
		check(sid, can);
	}
	printf("SAME %d p2 sleeps %d tick %u\n", id, P2_SLEEPS, (unsigned int) qd_ticks());

	/* Phase 3: immediate yields, each a scheduler pass and, with two copies runnable, a CR3
	 * switch; the timer keeps interrupting in between. */
	for (i = 0; i < P3_YIELDS; i++) {
		qd_yield(0);
		x += step;
		n++;
		if ((i & 31) == 0)
			check(sid, can);
	}
	check(sid, can);
	if (x != step * (double) n) {
		printf("SAME %d: x87 accumulator wrong after yields\n", id);
		bad++;
	}
	printf("SAME %d p3 yields %d tick %u\n", id, P3_YIELDS, (unsigned int) qd_ticks());
	info_line("end");
	printf("SAME %d done: n %u acc %08x checks %u bad %u first %u last %u\n", id,
	       (unsigned int) n, (unsigned int) acc, (unsigned int) checks, (unsigned int) bad,
	       (unsigned int) t0, (unsigned int) qd_ticks());
	return bad == 0 ? 0x50 + id : 0x5F;
}
