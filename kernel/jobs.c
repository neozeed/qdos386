/*
 * QDOS/386 R4 - job table, scheduler, service traps and job memory.
 *
 * Scheduling follows the JS ROM (system_asm.s), as in R2:
 *  - Timer interrupt (l00900): SV.POLLM is incremented, then the poll list
 *    runs, then the scheduler (l00936).
 *  - Scheduler pass (l0093a): SV.POLLM is read and cleared, SV.RAND is
 *    incremented, the schedule list runs (l00a9e), and a job is selected
 *    (l00a0c).  Then the chosen job is restored (l00a78).
 *  - Selection (l00a0c): for each candidate after the current job, with the
 *    current job last:
 *      skip if prio_inc == 0;
 *      if status > 0: status -= POLLM; still suspended while status > 0;
 *        on expiry, status = 0 and the hold flag is set to 0xFF;
 *      skip if status < 0 (indefinitely suspended);
 *      acc = 1 if it was 0, else acc + inc (saturating at 255);
 *      the strictly highest acc wins; ties go to the first candidate found.
 *  - Save (l009d4): a non-zero accumulated priority of the outgoing job is
 *    reset to 1.
 *
 * R3 memory rules:
 *  - A job's stack and every block it allocates with QT_MEM_ALLOC are heap
 *    blocks owned by the job's tagged id.  Exit reclaims them all.
 *  - A terminated job is marked dead inside its own scheduler pass, after
 *    its frame is saved, so it can never be running or resumed when its
 *    memory is freed.  Reclaim runs only in trap or cooperative-yield context
 *    of another job.  It never runs in the timer ISR.
 *  - Stack canaries are checked on every switch-out.  The check only counts
 *    failures, since the ISR must not log.  The SYSTEM job reads the count.
 *
 * Not reproduced: the job-header address table (SV.JBBAS) and the poll
 * wait flag (JOB.WFLAG), since slots are indices here.  Activate's wait
 * option is not implemented.
 */
#include "jobs.h"
#include "memory.h"
#include "channel.h"

struct job jobs[JOB_MAX];
int job_current = JOB_SYSTEM;
int job_idle = 3;
volatile unsigned long sched_ticks = 0;
volatile unsigned long sched_yields = 0;
volatile unsigned long sched_passes = 0;
volatile unsigned long poll_runs = 0;
volatile unsigned long sched_runs = 0;
volatile unsigned int sv_pollm = 0;
volatile unsigned long stack_faults = 0;
unsigned long jobs_reclaimed = 0;
unsigned long bytes_reclaimed = 0;
unsigned int sv_rand = 0;

static struct qtask *poll_list = 0;     /* SV.PLIST */
static struct qtask *sched_list = 0;    /* SV.SHLST */
static unsigned int dead_mask = 0;      /* slots whose frame is saved and dead */

extern void job_trampoline(void);
extern unsigned int irq_save(void);

/* Build a cooperative (format 0) initial frame: edi, esi, ebx, ebp, ret. */
static void init_frame(struct job *j)
{
	unsigned int *sp = (unsigned int *) j->stack_top;
	sp -= 5;
	sp[0] = 0;
	sp[1] = 0;
	sp[2] = 0;
	sp[3] = 0;
	sp[4] = (unsigned int) job_trampoline;
	j->saved_sp = (unsigned int) sp;
	j->saved_fmt = 0;
}

static void run_tasks(struct qtask *t, unsigned long *counter)
{
	while (t != 0) {
		(*counter)++;
		t->fn();
		t = t->next;
	}
}

/* Stack use in bytes: distance from the top to the lowest non-zero word
 * above the canary.  Bounded by JOB_STACK_BYTES.  Called from job context. */
static unsigned int stack_high_water(struct job *j)
{
	unsigned int a;
	for (a = j->stack_base + 16; a < j->stack_top; a += 4) {
		if (*(unsigned int *) a != 0)
			return j->stack_top - a;
	}
	return 0;
}

/* Free the resources of each dead job that is not running.  Called only
 * from trap or cooperative-yield context (never from the timer ISR). */
static void reclaim_dead(void)
{
	int i;
	for (i = 1; i < JOB_MAX; i++) {
		unsigned int bit = 1u << i;
		struct job *j = &jobs[i];
		if (i == job_current || !(dead_mask & bit))
			continue;
		if (j->stack_base != 0)
			j->stack_hwm = stack_high_water(j);
		/* Channels first: they free their own objects, so the heap pass
		 * below finds only the job's stack and user blocks. */
		chan_close_owner(j->id);
		bytes_reclaimed += mem_reclaim_owner(j->id);
		jobs_reclaimed++;
		dead_mask &= ~bit;
		j->in_use = 0;
		j->dead = 0;
		j->prio_inc = 0;
		j->acc_prio = 0;
		j->hold = 0;
		j->stack_base = 0;
		j->stack_top = 0;
	}
}

/* l00a0c with the timeout countdown of JOB.STAT */
static int pick_next(unsigned int pollm)
{
	int best = JOB_NONE;
	unsigned int best_acc = 0;
	int n, i;
	struct job *j;

	for (n = 1; n <= JOB_MAX; n++) {
		i = (job_current + n) % JOB_MAX;
		j = &jobs[i];
		if (!j->in_use || j->prio_inc == 0)
			continue;
		if (j->status > 0) {
			j->status -= (int) pollm;
			if (j->status > 0)
				continue;
			j->status = 0;
			if (j->hold != 0)
				*j->hold = 0xff;          /* sf (a1): release the flag */
		}
		if (j->status < 0)
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
	if (best == JOB_NONE)
		best = job_idle;
	return best;
}

/* Scheduler pass (l00936 .. l00a78).  Runs in the ISR or in a trap or yield
 * with IF off.  It does not allocate or free memory. */
static unsigned int schedule(unsigned int sp, int fmt, int from_irq)
{
	struct job *out = &jobs[job_current];
	unsigned int pollm;
	int next;

	out->saved_sp = sp;
	out->saved_fmt = fmt;
	if (out->dead)
		dead_mask |= 1u << job_current;    /* frame saved: now reclaimable */
	if (out->acc_prio != 0)
		out->acc_prio = 1;                 /* l009d4 */
	if (out->stack_base != 0 &&
	    (*(unsigned int *) out->stack_base != JOB_STACK_CANARY ||
	     *(unsigned int *) (out->stack_base + 4) != JOB_STACK_CANARY ||
	     *(unsigned int *) (out->stack_base + 8) != JOB_STACK_CANARY ||
	     *(unsigned int *) (out->stack_base + 12) != JOB_STACK_CANARY))
		stack_faults++;

	sched_passes++;
	pollm = sv_pollm;                      /* l0093a: read and clear POLLM */
	sv_pollm = 0;
	sv_rand++;
	run_tasks(sched_list, &sched_runs);    /* l00a9e: schedule list */

	next = pick_next(pollm);
	if (next != job_current) {
		if (from_irq && out->prio_inc != 0 && out->status == 0)
			out->preempts++;
		job_current = next;
	}
	jobs[job_current].dispatches++;
	return jobs[job_current].saved_sp | (unsigned int) jobs[job_current].saved_fmt;
}

/* IRQ0 path (l00900): poll list, then scheduler. */
unsigned int sched_irq_tick(unsigned int sp)
{
	sched_ticks++;
	timer_ack();
	sv_pollm++;
	run_tasks(poll_list, &poll_runs);
	return schedule(sp, 1, 1);
}

/* Cooperative yield path, called from yield_asm with IF off. */
unsigned int sched_yield_sp(unsigned int sp)
{
	sched_yields++;
	reclaim_dead();
	return schedule(sp, 0, 0);
}

int job_lookup(unsigned int id)
{
	unsigned int slot = id & 0xffffu;
	unsigned int tag = id >> 16;

	if (slot >= JOB_MAX)
		return JOB_NONE;
	if (!jobs[slot].in_use || jobs[slot].tag != tag)
		return JOB_NONE;
	return (int) slot;
}

unsigned int job_current_id(void)
{
	return jobs[job_current].id;
}

int stack_canary_ok(int slot)
{
	unsigned int b = jobs[slot].stack_base;
	if (b == 0)
		return 1;
	return *(unsigned int *) b == JOB_STACK_CANARY &&
	       *(unsigned int *) (b + 4) == JOB_STACK_CANARY &&
	       *(unsigned int *) (b + 8) == JOB_STACK_CANARY &&
	       *(unsigned int *) (b + 12) == JOB_STACK_CANARY;
}

/*
 * Allocate the lowest free slot >= 1, its heap stack, and its initial frame.
 * The stack is allocated before the slot changes, so a failure leaves the
 * slot, the tag and the heap unchanged.  Returns the job id, -1 if no slot
 * is free, or a negative MEM_ERR_* code if the stack cannot be allocated.
 */
int job_alloc(const char *name, void (*entry)(void), unsigned char inc)
{
	int i;
	struct job *j;
	unsigned int newtag;
	unsigned int id;
	unsigned int base;
	void *stk;

	for (i = 1; i < JOB_MAX; i++) {
		if (!jobs[i].in_use && !(dead_mask & (1u << i)))
			break;
	}
	if (i >= JOB_MAX)
		return -1;
	j = &jobs[i];

	newtag = j->tag + 1;
	if (newtag >= 0x7fffu)              /* keep ids positive; tag 0 is reserved */
		newtag = 1;
	id = (newtag << 16) | (unsigned int) i;

	stk = mem_alloc(JOB_STACK_BYTES, id);
	if (mem_is_error((unsigned int) stk))
		return (int) (unsigned int) stk;
	base = (unsigned int) stk;
	*(unsigned int *) base = JOB_STACK_CANARY;
	*(unsigned int *) (base + 4) = JOB_STACK_CANARY;
	*(unsigned int *) (base + 8) = JOB_STACK_CANARY;
	*(unsigned int *) (base + 12) = JOB_STACK_CANARY;

	j->tag = newtag;
	j->id = id;
	j->in_use = 1;
	j->dead = 0;
	j->name = name;
	j->entry = entry;
	j->prio_inc = inc;
	j->acc_prio = 0;
	j->status = 0;
	j->hold = 0;
	j->preempts = 0;
	j->dispatches = 0;
	j->stack_base = base;
	j->stack_top = base + JOB_STACK_BYTES;
	j->stack_hwm = 0;
	init_frame(j);
	return (int) id;
}

void job_idle_set(int slot)
{
	job_idle = slot;
}

void sched_init(void)
{
	int i;
	for (i = 0; i < JOB_MAX; i++) {
		jobs[i].saved_sp = 0;
		jobs[i].saved_fmt = 0;
		jobs[i].prio_inc = 0;
		jobs[i].acc_prio = 0;
		jobs[i].status = 0;
		jobs[i].hold = 0;
		jobs[i].tag = 0;
		jobs[i].id = 0;
		jobs[i].in_use = 0;
		jobs[i].dead = 0;
		jobs[i].name = 0;
		jobs[i].entry = 0;
		jobs[i].stack_base = 0;
		jobs[i].stack_top = 0;
		jobs[i].stack_hwm = 0;
		jobs[i].preempts = 0;
		jobs[i].dispatches = 0;
	}
	/* SYSTEM: the boot context, job 0.  Its stack is the boot stack, which
	 * is not on the heap, so it has no canary and is never reclaimed. */
	jobs[JOB_SYSTEM].in_use = 1;
	jobs[JOB_SYSTEM].prio_inc = 0x20;         /* JS ROM BASIC job increment */
	jobs[JOB_SYSTEM].name = "SYSTEM";
	jobs[JOB_SYSTEM].id = 0;
	job_current = JOB_SYSTEM;
	dead_mask = 0;
}

/*
 * Termination path (job_run returns here).  The dead flag is set with IF off,
 * so the timer cannot switch away before the frame is saved; schedule() then
 * marks the slot reclaimable.  The frame is never resumed.
 */
void job_exit(void)
{
	struct job *j = &jobs[job_current];
	(void) irq_save();
	j->prio_inc = 0;                  /* no longer active */
	j->status = -1;
	j->dead = 1;
	yield_asm();
	for (;;)
		;
}

void job_run(void)
{
	jobs[job_current].entry();
	job_exit();
}

/* Service traps.  EAX/EBX/ECX/EDX are in the interrupt frame: see context.s. */
unsigned int trap_dispatch(unsigned int sp)
{
	unsigned int *f = (unsigned int *) sp;
	unsigned int fn = f[7];               /* EAX */
	unsigned int a = f[4];                /* EBX */
	unsigned int c = f[5];                /* EDX */
	unsigned int b = f[6];                /* ECX */
	int result = -1;
	int slot;
	struct job *j;
	struct mem_stats st;

	reclaim_dead();

	switch (fn) {
	case QT_SUSPEND:
		/* tr1d08: JOB.STAT = timeout, JOB.HOLD = release flag, then schedule */
		jobs[job_current].status = (int) a;
		jobs[job_current].hold = (unsigned char *) b;
		result = 0;
		f[7] = (unsigned int) result;
		return schedule(sp, 1, 0);

	case QT_REACTIVATE:
		/* tr1d09: clear JOB.STAT and release the flag if one is set */
		slot = job_lookup(a);
		if (slot == JOB_NONE) {
			result = -2;
			break;
		}
		j = &jobs[slot];
		if (j->status != 0) {
			j->status = 0;
			if (j->hold != 0)
				*j->hold = 0xff;
		}
		result = 0;
		f[7] = (unsigned int) result;
		return schedule(sp, 1, 0);

	case QT_ACTIVATE:
		/* tr1d0a: fails with -1 if the job is already active */
		slot = job_lookup(a);
		if (slot == JOB_NONE) {
			result = -2;
			break;
		}
		j = &jobs[slot];
		if (j->prio_inc != 0) {
			result = -1;
			break;
		}
		j->prio_inc = (unsigned char) b;
		j->acc_prio = 0;
		result = 0;
		break;

	case QT_CHPRIO:
		/* tr1d0b / o990: set increment; zero the accumulated priority if 0 */
		slot = job_lookup(a);
		if (slot == JOB_NONE) {
			result = -2;
			break;
		}
		j = &jobs[slot];
		j->prio_inc = (unsigned char) b;
		if (b == 0)
			j->acc_prio = 0;
		result = 0;
		break;

	case QT_INFO:
		slot = job_lookup(a);
		if (slot == JOB_NONE) {
			result = -2;
			break;
		}
		j = &jobs[slot];
		result = (int) ((unsigned int) j->prio_inc |
		                ((unsigned int) j->acc_prio << 8));
		break;

	case QT_CREATE:
		result = job_alloc((const char *) c, (void (*)(void)) a,
		                   (unsigned char) b);
		break;

	case QT_POLL_ADD:
		((struct qtask *) a)->next = poll_list;
		poll_list = (struct qtask *) a;
		result = 0;
		break;

	case QT_SCHED_ADD:
		((struct qtask *) a)->next = sched_list;
		sched_list = (struct qtask *) a;
		result = 0;
		break;

	case QT_MEM_ALLOC:
		/* Owner is the calling job.  A pointer is returned as an unsigned
		 * value; error codes are negative and never valid pointers. */
		result = (int) (unsigned int) mem_alloc(a, job_current_id());
		break;

	case QT_MEM_FREE:
		result = mem_free((void *) a, job_current_id());
		break;

	case QT_MEM_FREE_BYTES:
		if (mem_stats(&st) == MEM_OK)
			result = (int) st.free;
		else
			result = MEM_ERR_CORRUPT;
		break;

	case QT_MEM_VALIDATE:
		result = mem_validate();
		break;

	case QT_CH_OPEN:
		result = chan_open(job_current_id(), a);
		break;

	case QT_CH_CLOSE:
		result = chan_close(a, job_current_id());
		break;

	case QT_CH_WRITE:
		result = chan_write(a, job_current_id(), b, c);
		break;

	case QT_CH_PUTC:
		result = chan_putc(a, job_current_id(), b);
		break;

	default:
		result = -1;
		break;
	}
	f[7] = (unsigned int) result;
	return sp | 1u;                       /* same frame, no scheduling */
}
