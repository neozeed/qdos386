/* QDOS/386 R14 QL compatibility layer: mt_cjob children as cooperative co-jobs, and the mt_* calls.
 *
 * QDOS jobs created with MT.CJOB share nothing by default, but QLTerm's children execute C code of the parent and
 * share its globals (the parent writes a 68000 "JMP.L entry" into the child's code area).  QDOS/386 programs have
 * private address spaces, so a kernel job cannot share the parent's globals.  The children therefore run as
 * co-jobs: separate C stacks inside the calling process, switched only at QDOS calls that wait (mt_susjb, and the
 * waits inside io_fbyte / io_fstrg / io_sstrg / io_edlin).  When every co-job waits, the process sleeps in the
 * kernel until the earliest deadline, serial data or a key (qd_ser_wait), so nothing spins.
 * mt_activ accepts only the exact stub QLTerm writes (0x4EF9 + a 32-bit address); anything else is ERR_BP.
 * Priorities: 0 stops a co-job (it is not scheduled), any other value runs it; co-jobs run round-robin. */
#include <stdlib.h>
#include <string.h>
#include <qdos386.h>
#include "qlc.h"

#define QLC_COJOBS 6
#define QLC_STACK  32768u

struct cojob {
	int used, prio, started;
	unsigned long sp;                  /* saved stack pointer while switched out */
	unsigned long wake;                /* qd_ticks() value it waits for */
	int waiting;
	void (*entry)(void);
	unsigned char *code;               /* the "code area" mt_cjob handed out */
	unsigned char *stack;
	char name[24];
};

static struct cojob cj[QLC_COJOBS];   /* 0 = the program's own (main) co-job */
static int cur = 0;

int qlc_cojob_current(void) { return cur; }

#ifdef QLC_HOST_TEST
#include <ucontext.h>
static ucontext_t uc[QLC_COJOBS];
static void trampoline(void);
static void sw(int from, int to) { swapcontext(&uc[from], &uc[to]); }
static void prepare(int k)
{
	getcontext(&uc[k]);
	uc[k].uc_stack.ss_sp = cj[k].stack;
	uc[k].uc_stack.ss_size = QLC_STACK;
	uc[k].uc_link = 0;
	makecontext(&uc[k], trampoline, 0);
}
#else
/* i386: save EBP EBX ESI EDI and ESP of 'from', load those of 'to'.  The compiler's own frame (push ebp; mov esp,ebp
 * ... leave; ret) is identical for every co-job, so after the load the epilogue returns into 'to'. */
static void co_switch(unsigned long *from, unsigned long to)
{
	asm volatile ("pushl %%ebp\n\tpushl %%ebx\n\tpushl %%esi\n\tpushl %%edi\n\tmovl %%esp,(%0)\n\t"
		      "movl %1,%%esp\n\tpopl %%edi\n\tpopl %%esi\n\tpopl %%ebx\n\tpopl %%ebp"
		      : : "a" (from), "d" (to));
}
static void trampoline(void);
static void sw(int from, int to) { co_switch(&cj[from].sp, cj[to].sp); }
/* a new stack: four saved registers, then a frame [ebp = 0][return = trampoline][return = 0] that co_switch's own
 * "leave; ret" consumes */
static void prepare(int k)
{
	unsigned long *top = (unsigned long *) (cj[k].stack + QLC_STACK) - 8;
	unsigned long *frame = top + 4;

	frame[0] = 0;
	frame[1] = (unsigned long) trampoline;
	frame[2] = 0;
	top[0] = 0; top[1] = 0; top[2] = 0;                /* edi esi ebx */
	top[3] = (unsigned long) frame;                     /* ebp: co_switch's leave makes esp = frame */
	cj[k].sp = (unsigned long) top;
}
#endif

static void trampoline(void)
{
	cj[cur].started = 1;
	cj[cur].entry();
	/* a QLTerm child never returns; if one does, it stops */
	cj[cur].prio = 0;
	for (;;)
		qlc_sleep(1000);
}

static int runnable(int k, unsigned long now)
{
	return cj[k].used && cj[k].prio != 0 && (!cj[k].waiting || (long) (now - cj[k].wake) >= 0);
}

/* the scheduler: called by a co-job that waits; returns when it is that co-job's turn again */
void qlc_sleep(long ticks)
{
	unsigned long now = qd_ticks(), early;
	int k, n, me = cur, any;

	cj[me].used = 1;
	if (me == 0 && cj[0].prio == 0)
		cj[0].prio = 1;
	cj[me].waiting = 1;
	cj[me].wake = now + (unsigned long) (ticks > 0 ? ticks : 0);
	for (;;) {
		now = qd_ticks();
		for (n = 1; n <= QLC_COJOBS; n++) {          /* round-robin from the next one; 'me' comes last */
			k = (me + n) % QLC_COJOBS;
			if (runnable(k, now)) {
				cj[k].waiting = 0;
				if (k != me) {
					cur = k;
					sw(me, k);
					cur = me;
				}
				if (!cj[me].waiting || (long) (qd_ticks() - cj[me].wake) >= 0) {
					cj[me].waiting = 0;
					return;
				}
				break;
			}
		}
		if (n <= QLC_COJOBS)
			continue;
		/* every co-job waits: sleep in the kernel until the earliest deadline, or serial data / a key */
		any = 0;
		early = now + 50;
		for (k = 0; k < QLC_COJOBS; k++)
			if (cj[k].used && cj[k].prio != 0 && cj[k].waiting && (!any || (long) (cj[k].wake - early) < 0)) {
				early = cj[k].wake;
				any = 1;
			}
		if ((long) (early - now) > 0) {
			/* wake early only for NEW input: data or keys already queued but not taken by any co-job (a stopped
			 * receive job, a menu not reading) must not end the sleep at once, or the process would spin */
			unsigned int fl = 0;
			if (qlc_ser > 0 && qd_ser_wait(qlc_ser, QD_SER_RX, 0) != (int) QD_SER_RX)
				fl |= QD_SER_RX;
			if (qlc_ser > 0 && qd_con_queued(qlc_con) == 0)
				fl |= QD_SER_KEY;
			if (fl)
				qd_ser_wait(qlc_ser, fl, early - now);
			else
				qd_yield((unsigned int) (early - now));
		}
	}
}

static int slot_of(jobid_t id)
{
	if (id == -1 || id == 0)
		return cur;
	if (id >= 0x200 && id < 0x200 + QLC_COJOBS && cj[id - 0x200].used)
		return (int) (id - 0x200);
	return -1;
}

jobid_t mt_cjob(long code, long data, char *start, jobid_t owner, char **base)
{
	int k;
	(void) data; (void) owner;
	if (start != 0)
		return ERR_BP;                 /* QLTerm passes no start address: the entry comes with mt_activ's stub */
	for (k = 1; k < QLC_COJOBS && cj[k].used; k++)
		;
	if (k == QLC_COJOBS)
		return ERR_OM;
	memset(&cj[k], 0, sizeof cj[k]);
	cj[k].code = (unsigned char *) malloc((unsigned int) (code < 64 ? 64 : code));
	cj[k].stack = (unsigned char *) malloc(QLC_STACK);
	if (cj[k].code == 0 || cj[k].stack == 0) {
		free(cj[k].code); free(cj[k].stack);
		return ERR_OM;
	}
	memset(cj[k].code, 0, (unsigned int) (code < 64 ? 64 : code));
	cj[k].used = 1;
	cj[0].used = 1;
	if (cj[0].prio == 0)
		cj[0].prio = 1;
	*base = (char *) cj[k].code;
	return 0x200 + k;
}

int mt_activ(jobid_t id, unsigned char prio, timeout_t t)
{
	int k = slot_of(id), n;
	unsigned char *p;
	(void) t;
	if (k <= 0 || cj[k].entry != 0)
		return ERR_NJ;
	p = cj[k].code;
	if (p[0] != 0x4E || p[1] != 0xF9)       /* JMP.L abs.L */
		return ERR_BP;
	cj[k].entry = (void (*)(void)) (((unsigned long) p[2] << 24) | ((unsigned long) p[3] << 16) |
				       ((unsigned long) p[4] << 8) | p[5]);
	if (cj[k].entry == 0)
		return ERR_BP;
	if (p[6] == 0x4A && p[7] == 0xFB) {     /* the QDOS job header: name */
		n = (p[8] << 8) | p[9];
		if (n > 23) n = 23;
		memcpy(cj[k].name, p + 10, (unsigned int) n);
		cj[k].name[n] = 0;
	}
	prepare(k);
	cj[k].prio = prio ? prio : 1;
	return 0;
}

int mt_prior(jobid_t id, int prio)
{
	int k = slot_of(id);
	if (k < 0)
		return ERR_NJ;
	cj[k].used = 1;
	cj[k].prio = prio;
	if (k == cur && prio == 0)
		qlc_sleep(0);                  /* stopping oneself: never happens in QLTerm, but do not keep running */
	return 0;
}

int mt_susjb(jobid_t id, timeout_t frames, char *flag)
{
	(void) flag;
	if (id != -1 && slot_of(id) != cur)
		return ERR_NI;                 /* suspending another co-job is not used by QLTerm */
	qlc_sleep(frames < 0 ? 0x7fffffffL : 2L * frames);
	return 0;
}

int mt_frjob(jobid_t id, int err)
{
	if (id != -1 && id != 0)
		return ERR_NI;
	exit(err < 0 ? 1 : err);
	return 0;
}

long mt_rclck(void)
{
	unsigned long t = qd_wall_time();
	return t == 0xFFFFFFFFUL ? 0L : (long) (t + 283996800UL);
}

/* MT.INF: a private block standing for the system variables QLTerm reads: +0x48 driver list (none), +0xB0 the
 * DATA_USE default directory (a QDOS string: the current directory as WIN1_DIR/), +0xEE pending writes (0). */
static unsigned char sysvars[0x100];
static char datause[2 + 70];
int mt_inf(char **sys, long *ver)
{
	char cwd[64];
	int n = qd_getcwd(cwd, sizeof cwd), k = 0;

	memset(sysvars, 0, sizeof sysvars);
	strcpy(datause + 2, "WIN1_");
	k = 5;
	if (n > 1 && n < 60) {
		strcpy(datause + 2 + k, cwd + 1);
		k += n - 1;
		datause[2 + k++] = '/';
	}
	datause[0] = (char) (k >> 8);
	datause[1] = (char) k;
	*(char **) (sysvars + 0xB0) = datause;
	*sys = (char *) sysvars;
	*ver = 0;                            /* no QDOS version: this is not QDOS */
	return qd_self();
}

int mt_dmode(short *mode, short *display)
{
	if (*mode != -1 || *display != -1)
		return ERR_NI;                 /* setting a mode is not supported; QLTerm only reads */
	*mode = 0;                           /* MODE 4 (the 4-colour mode QLTerm supports) */
	*display = 0;
	return 0;
}
