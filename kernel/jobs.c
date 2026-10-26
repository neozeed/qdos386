/*
 * QDOS/386 R1 - job table and scheduler.
 *
 * Selection follows the JS ROM accumulated-priority scheme (system_asm.s,
 * l00a0c and l009d4):
 *   - Each runnable job with a non-zero increment is examined once per
 *     scheduling pass.  Its accumulated priority becomes 1 if it was 0,
 *     otherwise it grows by its increment, saturating at 255.
 *   - The job with the strictly highest accumulated priority is chosen.
 *     Ties go to the first candidate after the current job in table order.
 *   - When a job is descheduled while still runnable, a non-zero accumulated
 *     priority is reset to 1 (l009d4).
 *
 * Not reproduced in R1: suspend timeouts (SV.POLLM countdown), the job tag
 * check, linked poll and schedule task lists, trap dispatch, and the
 * release flag handling for job holds.
 *
 * Scheduling runs on two paths: timer preemption (sched_irq_tick) and
 * cooperative yield (sched_yield_sp).  Each returns (saved_sp | fmt).
 */
#include "jobs.h"

struct job jobs[JOB_MAX];
int job_current = 0;
unsigned long sched_ticks = 0;
unsigned long sched_yields = 0;

static int job_count = 0;

/* Choose the next job.  The current job is examined last, as in l00a0c. */
static int pick_next(void)
{
	int best = -1;
	unsigned int best_acc = 0;
	int n, i;
	struct job *j;

	for (n = 1; n <= job_count; n++) {
		i = (job_current + n) % job_count;
		j = &jobs[i];
		if (j->status != JOB_RUN || j->prio_inc == 0)
			continue;
		if (j->acc_prio == 0) {
			j->acc_prio = 1;
		} else {
			unsigned int a = (unsigned int) j->acc_prio + j->prio_inc;
			j->acc_prio = (a > 255) ? 255 : a;
		}
		if ((unsigned int) j->acc_prio > best_acc) {
			best_acc = j->acc_prio;
			best = i;
		}
	}
	if (best < 0)
		best = JOB_IDLE_INDEX;      /* never reached while idle is runnable */
	return best;
}

/* Shared body: save outgoing state, select, return the incoming state. */
static unsigned int schedule(unsigned int sp, int fmt)
{
	struct job *out = &jobs[job_current];
	int next;

	out->saved_sp = sp;
	out->saved_fmt = fmt;
	if (out->acc_prio != 0)
		out->acc_prio = 1;          /* l009d4: reset accumulated priority */

	next = pick_next();
	if (next != job_current) {
		if (fmt == 1 && out->status == JOB_RUN)
			out->preempts++;
		job_current = next;
	}
	jobs[job_current].dispatches++;
	return jobs[job_current].saved_sp | (unsigned int) jobs[job_current].saved_fmt;
}

unsigned int sched_irq_tick(unsigned int sp)
{
	sched_ticks++;
	return schedule(sp, 1);
}

unsigned int sched_yield_sp(unsigned int sp)
{
	sched_yields++;
	return schedule(sp, 0);
}

/*
 * Build a fmt-0 initial frame so that the first dispatch pops four zero
 * registers and returns into job_trampoline.  Layout from low address:
 *   edi, esi, ebx, ebp, return address (job_trampoline).
 */
extern void job_trampoline(void);

static void init_frame(struct job *j)
{
	unsigned int *sp = (unsigned int *) j->stack_top;
	sp -= 5;
	sp[0] = 0;                              /* edi */
	sp[1] = 0;                              /* esi */
	sp[2] = 0;                              /* ebx */
	sp[3] = 0;                              /* ebp */
	sp[4] = (unsigned int) job_trampoline;  /* return address */
	j->saved_sp = (unsigned int) sp;
	j->saved_fmt = 0;
}

void sched_init(void)
{
	int i;
	for (i = 0; i < JOB_MAX; i++) {
		jobs[i].saved_sp = 0;
		jobs[i].saved_fmt = 0;
		jobs[i].prio_inc = 0;
		jobs[i].acc_prio = 0;
		jobs[i].status = JOB_DEAD;
		jobs[i].name = 0;
		jobs[i].entry = 0;
		jobs[i].stack_base = 0;
		jobs[i].stack_top = 0;
		jobs[i].preempts = 0;
		jobs[i].dispatches = 0;
	}
	job_count = 0;
	job_current = 0;
}

/*
 * Register a job in the next free table slot.  The first job registered
 * (slot 0) is the SYSTEM job, which is the boot context and needs no fresh
 * stack.  Its stack_base/stack_top stay zero.
 */
int job_create(const char *name, void (*entry)(void), unsigned char prio_inc,
               unsigned int stack_base, unsigned int stack_bytes)
{
	struct job *j;
	int slot = job_count;

	if (slot >= JOB_MAX)
		return -1;
	j = &jobs[slot];
	j->name = name;
	j->entry = entry;
	j->prio_inc = prio_inc;
	j->acc_prio = 0;
	j->status = JOB_RUN;
	j->preempts = 0;
	j->dispatches = 0;
	j->stack_base = stack_base;
	j->stack_top = stack_base + stack_bytes;
	if (entry != 0)
		init_frame(j);
	job_count++;
	return slot;
}

/* Used by SYSTEM (job 0) to register its boot context.  No frame is built. */
void job_register_boot(const char *name, unsigned char prio_inc)
{
	struct job *j = &jobs[job_count];
	j->name = name;
	j->entry = 0;
	j->prio_inc = prio_inc;
	j->acc_prio = 0;
	j->status = JOB_RUN;
	j->saved_sp = 0;
	j->saved_fmt = 0;
	job_count++;
}

void job_suspend_self(void)
{
	jobs[job_current].status = JOB_SUSPENDED;
	yield_asm();
}

/* Called by job_run after entry returns, or directly by a job. */
void job_exit(void)
{
	jobs[job_current].status = JOB_DEAD;
	yield_asm();
	/*
	 * A dead job is never selected again, so this point is not reached.
	 * Its stack is not reused in R1 (static pool, reclamation deferred).
	 */
	for (;;)
		;
}

/* Entry point of every fresh job, reached through job_trampoline. */
void job_run(void)
{
	jobs[job_current].entry();
	job_exit();
}
