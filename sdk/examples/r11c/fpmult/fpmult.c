/*
 * QDOS/386 R11C - two-job x87 proof (FPMULT).  Derived from the R10 FPJOB
 * test (tests/r10/fpjob.c, kept unchanged as a frozen fixture); the protocol
 * is the same, only the banner and the final line differ.
 *
 * Round 0 and round 2: two CPU-bound jobs with different x87 sums, with no
 * voluntary yield, so the timer alone switches between them.
 * Round 1: two jobs that yield with QT_SUSPEND every 1024 steps (the trap
 * path of the same scheduler).
 * Main keeps its own x87 state (control word 0x0c7f and two stack values)
 * across all rounds.  Each job must start with a clean x87 state, produce its
 * exact result, and exit with status 0.  Every value is exact in a double, so
 * any loss of x87 state shows as a wrong result.
 */
#include <stdio.h>
#include "qdos386.h"

#define QT_CREATE       0x0D
#define QT_JOB_RECLAIMED 0x1E
#define JOB_RUNNING     (-16)
#define STEPS           1500000

/* x87 helpers (top-level assembly, passed through by GCC 1.40).
 * fp_run keeps five values live on the x87 stack for the whole loop:
 *   ST0 = acc (the exact sum), ST1 = c (the step), ST2 = i (counter),
 *   ST3 = acc2 (a second counter, must equal steps), ST4 = K (canary, 2.5).
 * A timer switch or a cooperative yield in the middle of the loop must keep
 * all five intact. */
asm(".text\n"
    ".globl _fpu_snapshot\n"
    "_fpu_snapshot:\n"
    "\tmovl 4(%esp),%eax\n"
    "\tfnstenv (%eax)\n"
    "\tret\n"
    ".globl _fpu_setcw\n"
    "_fpu_setcw:\n"
    "\tfldcw 4(%esp)\n"
    "\tret\n"
    ".globl _fpu_pushjunk\n"
    "_fpu_pushjunk:\n"
    "\tfldl _fj75\n"
    "\tfldl _fj225\n"
    "\tret\n"
    ".globl _fpu_junk_read\n"
    "_fpu_junk_read:\n"
    "\tmovl 4(%esp),%eax\n"
    "\tfstpl (%eax)\n"
    "\tfstpl 8(%eax)\n"
    "\tret\n"
    ".globl _fp_run\n"
    "_fp_run:\n"
    "\tpushl %ebp\n"
    "\tmovl %esp,%ebp\n"
    "\tpushl %ebx\n"          /* -4  */
    "\tpushl %esi\n"          /* -8  */
    "\tpushl %edi\n"          /* -12 */
    "\tpushl $0\n"            /* -16 last progress seen from the other job */
    "\tpushl $0\n"            /* -20 seen: times the other job advanced */
    /* x87 stack on entry to the loop, top first:
     *   ST0 i (counter, 0..), ST1 acc (running sum), ST2 c (step),
     *   ST3 cnt (second counter), ST4 K (canary 2.5)
     * Pushed bottom-up: K, cnt=0, c, acc=0, i=0. */
    "\tfldl _fjk\n"
    "\tfldz\n"
    "\tfldl 12(%ebp)\n"
    "\tfldz\n"
    "\tfldz\n"
    "\tmovl 8(%ebp),%ebx\n"
    "\tmovl 20(%ebp),%esi\n"
    "\txorl %edi,%edi\n"
    "_fp_loop:\n"
    "\tfld %st(2)\n"          /* push c                 */
    "\tfmul %st(1),%st\n"     /* c*i                    */
    "\tfaddp %st,%st(2)\n"    /* acc += c*i, pop        */
    "\tfld1\n"
    "\tfaddp %st,%st(1)\n"    /* i += 1, pop            */
    "\tfld1\n"
    "\tfaddp %st,%st(4)\n"    /* cnt += 1, pop          */
    "\tincl %edi\n"
    "\ttestl $1023,%edi\n"
    "\tjne _fp_noyield\n"
    /* every 1024 steps: publish our progress, and count the other job's */
    "\tmovl 28(%ebp),%ecx\n"  /* mine  */
    "\tmovl %edi,(%ecx)\n"
    "\tmovl 32(%ebp),%ecx\n"  /* other */
    "\tmovl (%ecx),%eax\n"
    "\tcmpl -16(%ebp),%eax\n"
    "\tje _fp_same\n"
    "\tmovl %eax,-16(%ebp)\n"
    "\tincl -20(%ebp)\n"
    "_fp_same:\n"
    "\ttestl %esi,%esi\n"
    "\tje _fp_noyield\n"
    "\tpushl $0\n"            /* c */
    "\tpushl $0\n"            /* b */
    "\tpushl $1\n"            /* a = timeout ticks */
    "\tpushl $8\n"            /* fn = QT_SUSPEND */
    "\tcall _qtrap\n"
    "\taddl $16,%esp\n"
    "_fp_noyield:\n"
    "\tdecl %ebx\n"
    "\tjne _fp_loop\n"
    /* stack: i, acc, c, cnt, K */
    "\tmovl 24(%ebp),%eax\n"
    "\tfstpl 16(%eax)\n"      /* out[4..5] = i   */
    "\tfstpl (%eax)\n"        /* out[0..1] = acc */
    "\tfstp %st(0)\n"         /* drop c          */
    "\tfstpl 8(%eax)\n"       /* out[2..3] = cnt */
    "\tfstpl 24(%eax)\n"      /* out[6..7] = K   */
    "\tmovl -20(%ebp),%eax\n" /* return seen     */
    "\tleal -12(%ebp),%esp\n"
    "\tpopl %edi\n"
    "\tpopl %esi\n"
    "\tpopl %ebx\n"
    "\tpopl %ebp\n"
    "\tret\n"
    ".data\n"
    "_fj75:\n"
    "\t.long 0x00000000, 0x401e0000\n"          /* 7.5 */
    "_fj225:\n"
    "\t.long 0x00000000, 0x40020000\n"          /* 2.25 */
    "_fjk:\n"
    "\t.long 0x00000000, 0x40040000\n"          /* 2.5 (canary) */
    ".text\n");

extern void fpu_snapshot(unsigned int *out);     /* out[0] CW, out[1] SW, out[2] TW */
extern void fpu_setcw(unsigned int cw);
extern void fpu_pushjunk(void);
extern void fpu_junk_read(unsigned int *out);     /* out[0..1] = 2.25, out[2..3] = 7.5 */
extern int fp_run(unsigned int steps, double c, int coop, unsigned int *out, unsigned int *mine, unsigned int *other);
static unsigned int prog[4];   /* per-job progress, published every 1024 steps */

struct jres {
	int clean;                  /* clean x87 state at job start */
	int exact;                  /* result equals the exact sum */
	int done;
	unsigned int bits[2];
	unsigned int snapw[3];
	int seen;
};
static struct jres jr[4];

static void bits_of(double d, unsigned int *w)
{
	union { double d; unsigned int w[2]; } u;
	u.d = d;
	w[0] = u.w[0];
	w[1] = u.w[1];
}

static double dbl_of(unsigned int *w)
{
	union { double d; unsigned int w[2]; } u;
	u.w[0] = w[0];
	u.w[1] = w[1];
	return u.d;
}

static void run_fp(int which, double c, int coop)
{
	unsigned int snap[8], out[8];
	double want = c * ((double) STEPS * (double) (STEPS - 1) / 2.0);
	double acc, cnt, acc2, canary;

	fpu_snapshot(snap);
	/* Clean start: CW = FNINIT value, SW = 0 (TOP = 0, no exception flags).
	 * The tag word is printed but not checked: i386VM's FNSTENV stores 0 there
	 * whatever the stack holds (main's two live values also read 0000). */
	jr[which].clean = ((snap[0] & 0xffffu) == 0x037fu) &&
	                  ((snap[1] & 0xffffu) == 0);
	jr[which].snapw[0] = snap[0];
	jr[which].snapw[1] = snap[1];
	jr[which].snapw[2] = snap[2];
	jr[which].seen = fp_run(STEPS, c, coop, out, &prog[which], &prog[which ^ 1]);
	acc = dbl_of(out);
	cnt = dbl_of(out + 2);
	acc2 = dbl_of(out + 4);
	canary = dbl_of(out + 6);
	jr[which].exact = (acc == want) && (cnt == (double) STEPS) &&
	                  (acc2 == (double) STEPS) && (canary == 2.5);
	bits_of(acc, jr[which].bits);
	jr[which].done = 1;
	qtrap(QD_TRAP_EXIT, (jr[which].clean && jr[which].exact) ? 0 : 1, 0, 0);
}

static void job0(void) { run_fp(0, 0.5, 0); }
static void job1(void) { run_fp(1, 0.25, 0); }
static void job2(void) { run_fp(2, 0.125, 1); }
static void job3(void) { run_fp(3, 0.0625, 1); }

static int wait_exit(unsigned int id)
{
	int st;
	while ((st = qtrap(QD_TRAP_JOB_EXIT, id, 0, 0)) == JOB_RUNNING)
		qtrap(QD_TRAP_SUSPEND, 1, 0, 0);
	return st;
}

static void wait_reclaimed(unsigned int id)
{
	while (qtrap(QT_JOB_RECLAIMED, id, 0, 0) != 1)
		qtrap(QD_TRAP_SUSPEND, 1, 0, 0);
}

int main(void)
{
	unsigned int tw0[8], tw1[8], junk[4], cw[8], id[2];
	unsigned long t0, t1;
	int round, k, st, ok = 1, main_ok;
	int a_ok[2];

	fpu_setcw(0x0c7fu);                 /* main: round-towards-zero, a distinct CW */
	fpu_pushjunk();                     /* main: two values on the x87 stack */
	fpu_snapshot(tw0);

	for (round = 0; round < 3; round++) {
		void (*fa)(void) = (round == 1) ? job2 : job0;
		void (*fb)(void) = (round == 1) ? job3 : job1;
		int ca = (round == 1) ? 2 : 0;
		int cb = (round == 1) ? 3 : 1;
		static const char *names[4] = { "FPMULT-0", "FPMULT-1", "FPMULT-2", "FPMULT-3" };

		prog[ca] = prog[cb] = 0;
		jr[ca].done = jr[cb].done = 0;
		jr[ca].clean = jr[cb].clean = 0;
		jr[ca].exact = jr[cb].exact = 0;
		t0 = qtrap(QD_TRAP_TICKS, 0, 0, 0);
		id[0] = qtrap(QT_CREATE, (unsigned int) fa, 2, (unsigned int) names[ca]);
		id[1] = qtrap(QT_CREATE, (unsigned int) fb, 2, (unsigned int) names[cb]);
		if ((int) id[0] <= 0 || (int) id[1] <= 0) {
			printf("FPMULT round %d: job creation failed (%d, %d)\n", round, (int) id[0], (int) id[1]);
			ok = 0;
			break;
		}
		st = wait_exit(id[0]);
		a_ok[0] = (st == 0);
		st = wait_exit(id[1]);
		a_ok[1] = (st == 0);
		wait_reclaimed(id[0]);
		wait_reclaimed(id[1]);
		t1 = qtrap(QD_TRAP_TICKS, 0, 0, 0);

		printf("FPMULT round %d (%s) ticks %lu\n", round, round == 1 ? "cooperative" : "cpu-bound", t1 - t0);
		printf("  job %d: clean=%d exact=%d exit=%d seen=%d snap cw=%04x sw=%04x tw=%04x bits=%08x%08x\n", ca, jr[ca].clean, jr[ca].exact, a_ok[0] ? 0 : 1, jr[ca].seen, jr[ca].snapw[0] & 0xffff, jr[ca].snapw[1] & 0xffff, jr[ca].snapw[2] & 0xffff,
		       jr[ca].bits[1], jr[ca].bits[0]);
		printf("  job %d: clean=%d exact=%d exit=%d seen=%d snap cw=%04x sw=%04x tw=%04x bits=%08x%08x\n", cb, jr[cb].clean, jr[cb].exact, a_ok[1] ? 0 : 1, jr[cb].seen, jr[cb].snapw[0] & 0xffff, jr[cb].snapw[1] & 0xffff, jr[cb].snapw[2] & 0xffff,
		       jr[cb].bits[1], jr[cb].bits[0]);
		if (jr[ca].seen < 8 || jr[cb].seen < 8)
			printf("  interleave check: seen %d and %d (need >= 8)\n", jr[ca].seen, jr[cb].seen);
		if (!(jr[ca].clean && jr[ca].exact && jr[cb].clean && jr[cb].exact && a_ok[0] && a_ok[1] && jr[ca].seen >= 8 && jr[cb].seen >= 8))
			ok = 0;
	}

	/* Main's own x87 state after all rounds. */
	fpu_snapshot(tw1);
	cw[0] = tw1[0] & 0xffffu;
	/* Main's stack top (SW bits 11..13) must still be the value after the two
	 * junk pushes, and its CW unchanged. */
	main_ok = (cw[0] == 0x0c7fu) && ((tw0[1] & 0x3800u) == (tw1[1] & 0x3800u));
	fpu_junk_read(junk);
	main_ok = main_ok && junk[1] == 0x40020000u && junk[0] == 0 && junk[3] == 0x401e0000u && junk[2] == 0;
	printf("FPMULT main: cw=%04x sw-top %x -> %x junk=%08x%08x %08x%08x\n", cw[0],
	       (tw0[1] >> 11) & 7, (tw1[1] >> 11) & 7, junk[1], junk[0], junk[3], junk[2]);
	printf("FPMULT main state %s\n", main_ok ? "intact" : "CHANGED");
	fpu_setcw(0x037fu);
	ok = ok && main_ok;
	printf("%s\n", ok ? "QDOS/386 R11C FPMULT PASS" : "QDOS/386 R11C FPMULT FAIL");
	return 0;
}
