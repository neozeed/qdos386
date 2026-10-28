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
#include "aout.h"
#include "cin.h"
#include "imgload.h"
#include "fpu.h"
#include "clock_boot.h"
#ifdef QDOS_FS_WRITE
#include "win1.h"
#endif
#ifdef QDOS_PAGING
#include "paging.h"
#endif
#ifdef QDOS_VCON
#include "vcon.h"
#endif
#ifdef QDOS_JOBCTL
#include "jobctl.h"
#endif

/* R11J: a hold-flag release.  In the paging POC the flag may live in a paged job's
 * private window, so the write is made in that job's address space (paging.c). */
#ifdef QDOS_PAGING
#define HOLD_RELEASE(slot, j) pg_hold_release(slot)
#define RUN_TASKS(t, c) run_tasks_template(t, c)
#else
#define HOLD_RELEASE(slot, j) (*(j)->hold = 0xff)
#define RUN_TASKS(t, c) run_tasks(t, c)
#endif

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

static struct job_exit_rec exit_rec[JOB_MAX];

/* QT_JOB_EXIT: the record is looked up by the full id, so a reused slot
 * (new tag) or a never-created id gives -2. */
static int job_exit_status(unsigned int id)
{
	int i = (int) (id & 0xffffu);
	if (i <= 0 || i >= JOB_MAX || exit_rec[i].id != id)
		return -2;
	if (exit_rec[i].exited)
		return (int) exit_rec[i].status;
#ifdef QDOS_JOBCTL
	if (jobctl_killed(id))
		return JC_ERR_KILLED;               /* R11M: ended by QT_JOB_KILL */
#endif
	if (jobs[i].in_use && !jobs[i].dead)
		return JOB_ERR_RUNNING;
	return JOB_ERR_NOSTATUS;
}

static void run_tasks(struct qtask *t, unsigned long *counter)
{
	while (t != 0) {
		(*counter)++;
		t->fn();
		t = t->next;
	}
}

#ifdef QDOS_PAGING
/* R11J: task nodes and code belong to unpaged registrants; run them in the template. */
static void run_tasks_template(struct qtask *t, unsigned long *counter)
{
	unsigned int was;

	if (t == 0)
		return;
	was = pg_enter_template();
	run_tasks(t, counter);
	pg_leave_template(was);
}
#endif

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
#ifdef QDOS_PAGING
		/* R11J: first the address space; if its directory were active, keep the slot dead
		 * and retry on a later pass (never reached: guard). */
		if (pg_release_owner(j->id, i) != 0)
			continue;
#endif
		if (j->stack_base != 0)
			j->stack_hwm = stack_high_water(j);
		/* Channels first: they free their own objects, so the heap pass
		 * below finds only the job's stack and user blocks. */
		chan_close_owner(j->id);
		app_release_owner(j->id);
		bytes_reclaimed += mem_reclaim_owner(j->id);
		jobs_reclaimed++;
#ifdef QDOS_VCON
		vcon_job_gone(i);                 /* R11M: its console waiter, session count and focus */
#endif
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
				HOLD_RELEASE(i, j);       /* sf (a1): release the flag */
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
	RUN_TASKS(sched_list, &sched_runs);    /* l00a9e: schedule list */

	next = pick_next(pollm);
	if (next != job_current) {
		if (from_irq && out->prio_inc != 0 && out->status == 0)
			out->preempts++;
		/* R10: the x87 state belongs to the job.  Save the outgoing job's
		 * (FNSAVE also reinitialises the FPU) and load the incoming one's.
		 * Both switch paths (timer ISR and cooperative yield) come through
		 * here, so both are covered. */
		fpu_save(out->fpu);
		job_current = next;
		fpu_restore(jobs[job_current].fpu);
#ifdef QDOS_PAGING
		/* R11J: the incoming job's address space.  IF is off, and every stack and the
		 * kernel are in the shared template mapping, so the switch may happen here. */
		pg_activate(job_current);
		pg_after_switch(job_current);
#endif
	}
	jobs[job_current].dispatches++;
	return jobs[job_current].saved_sp | (unsigned int) jobs[job_current].saved_fmt;
}

/* IRQ0 path (l00900): poll list, then scheduler. */
unsigned int sched_irq_tick(unsigned int sp)
{
	sched_ticks++;
	clock_tick();                          /* R11F: wall clock */
	timer_ack();
	sv_pollm++;
	cin_poll_tick();                       /* R7: bounded, no-op unless enabled */
	RUN_TASKS(poll_list, &poll_runs);
	return schedule(sp, 1, 1);
}

/* Console waiter (R7): the one job asleep in QT_CON_WAIT, or JOB_NONE.  Only
 * one job may wait on the console at a time.  A second job that asks while
 * the first is still asleep is refused with CON_ERR_BUSY; it does not replace
 * the waiter, so the first is never left asleep without a wake. */
#ifdef QDOS_VCON
/* R11M: one waiter per console, kept in kernel/vcon.c.  vcon_wake_job marks the sleeping job runnable
 * and releases its flag; like the single-console version it never schedules. */
void vcon_wake_job(int slot)
{
	struct job *j;

	if (slot < 0 || slot >= JOB_MAX)
		return;
	j = &jobs[slot];
	if (j->in_use && j->status < 0) {
		j->status = 0;
		if (j->hold != 0)
			HOLD_RELEASE(slot, j);
	}
}
#else
static int con_waiter = JOB_NONE;

/* Called by cin_put and cin_set_eof (IRQ1 or a trap, IF off).  It only marks
 * the waiter runnable and releases its flag, so it never schedules. */
void con_input_arrived(void)
{
	struct job *j;

	if (con_waiter == JOB_NONE)
		return;
	j = &jobs[con_waiter];
	if (j->in_use && j->status < 0) {
		j->status = 0;
		if (j->hold != 0)
			HOLD_RELEASE(con_waiter, j);
	}
	con_waiter = JOB_NONE;
}
#endif

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
	return job_alloc_sized(name, entry, inc, JOB_STACK_BYTES);
}

int job_alloc_sized(const char *name, void (*entry)(void), unsigned char inc, unsigned int stack_bytes)
{
	int i;
	struct job *j;
	unsigned int newtag;
	unsigned int id;
	unsigned int base;
	void *stk;

	if (stack_bytes < JOB_STACK_BYTES || stack_bytes > 65536u || (stack_bytes & 15u) != 0)
		return MEM_ERR_OVERFLOW;

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

	stk = mem_alloc(stack_bytes, id);
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
	fpu_clean(j->fpu);                 /* R10: a new job starts with a clean x87 */
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
	j->stack_top = base + stack_bytes;
	j->stack_bytes = stack_bytes;
	j->args = 0;
	j->stack_hwm = 0;
	exit_rec[i].id = id;
	exit_rec[i].exited = 0;
	exit_rec[i].status = 0;
	init_frame(j);
#ifdef QDOS_FS_WRITE
	win1_cwd_inherit(id, jobs[job_current].id);     /* R11M: the creator's current directory */
#endif
#ifdef QDOS_VCON
	vcon_job_inherit(i, job_current);               /* R11M: the creator's console (or its chosen child console) */
#endif
#ifdef QDOS_JOBCTL
	jobctl_slot_reset(i);
#endif
	return (int) id;
}

/* R8: release a job that never ran (its image copy failed).  Its heap blocks
 * are freed, and the slot is free at once: no frame was ever saved for it. */
void job_cancel(unsigned int id)
{
	int i = job_lookup(id);
	struct job *j;

	if (i == JOB_NONE)
		return;
	j = &jobs[i];
	(void) mem_reclaim_owner(j->id);
#ifdef QDOS_VCON
	vcon_job_gone(i);
#endif
	j->in_use = 0;
	j->dead = 0;
	j->prio_inc = 0;
	j->stack_base = 0;
	j->stack_top = 0;
	j->args = 0;
	exit_rec[i].id = 0;
	exit_rec[i].exited = 0;
}

#ifdef QDOS_JOBCTL
/* R11M: end another job.  Its frame is saved (it is not running), so it only has to be made inactive and dead; the
 * ordinary reclaim pass (next trap or yield, never the timer ISR) frees its address space, channels, heap and stack,
 * and refuses to free a directory that is still active (reclaim_dead / pg_release_owner). */
void job_mark_dead_other(int slot)
{
	struct job *j = &jobs[slot];

	if (slot == job_current || slot <= 0 || slot >= JOB_MAX || !j->in_use)
		return;
	j->prio_inc = 0;
	j->status = -1;
	j->dead = 1;
	dead_mask |= 1u << slot;
}
#endif

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
	fpu_clean(jobs[JOB_SYSTEM].fpu);
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

#ifdef QDOS_PAGING
	/* R11J: services whose pointers the kernel uses later, in other contexts, or that
	 * load images, are not available to a paged process (docs/R11J-ADDRESS-SPACE-ABI.md). */
	if (pg_slot_paged(job_current) &&
	    (fn == QT_CREATE || fn == QT_POLL_ADD || fn == QT_SCHED_ADD ||
#ifdef QDOS_JOBCTL
	     fn == QT_JOB_KILL ||
#endif
	     fn == QT_IMG_LOAD || fn == QT_IMG_START || fn == QT_PG_START || fn == QT_PG_CANCEL)) {
		f[7] = (unsigned int) PG_ERR_REFUSED;
		return sp | 1u;
	}
#endif

	switch (fn) {
	case QT_SUSPEND:
		/* tr1d08: JOB.STAT = timeout, JOB.HOLD = release flag, then schedule.
		 * R11E: the hold flag is written by the kernel later, so it must be a
		 * byte in the caller's range (0 = no flag). */
		if (b != 0 && !chan_user_range_ok(b, 1)) {
			result = CHAN_ERR_RANGE;
			break;
		}
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
				HOLD_RELEASE(slot, j);
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

	case QT_EXIT:
		exit_rec[job_current].id = jobs[job_current].id;
		exit_rec[job_current].exited = 1;
		exit_rec[job_current].status = a;
		result = 0;
		break;

	case QT_JOB_EXIT:
		result = job_exit_status(a);
		break;

	case QT_CH_READ:
		result = chan_read(a, job_current_id(), b, c);
		break;

	case QT_CH_SEEK:
		result = chan_seek(a, job_current_id(), b);
		break;

#ifdef QDOS_FS_WRITE
	case QT_CH_OPENX:
		result = chan_open_ex(job_current_id(), a, b);
		break;

	case QT_CH_CTL:
		result = chan_ctl(a, job_current_id(), b, c);
		break;

	case QT_FS_OP:
		result = chan_fsop(a, b, c);
		break;
#endif

	case QT_IMG_LOAD:
		result = img_load_request(a, b, c);
#ifdef QDOS_JOBCTL
		if (result > 0)
			jobctl_note_launch(result, a, 0);
#endif
		break;

	case QT_IMG_START:
		result = img_start_request(a, (int) b);
#ifdef QDOS_JOBCTL
		if (result > 0)
			jobctl_note_launch(result, a, 0);
#endif
		break;

	case QT_APP_ARGS:
		/* R8: argc of the calling job, and the argv array address through a.
		 * A job with no argument block has argc 0. */
		if (a != 0 && !chan_user_range_ok(a, 4)) {
			result = CHAN_ERR_RANGE;
			break;
		}
		if (jobs[job_current].args == 0) {
			if (a != 0)
				*(unsigned int *) a = 0;
			result = 0;
			break;
		}
		if (a != 0)
			*(unsigned int *) a = *(unsigned int *) (jobs[job_current].args + 4);
		result = (int) *(unsigned int *) jobs[job_current].args;
		break;

	case QT_TICKS:
		result = (int) (unsigned int) sched_ticks;
		break;

	case QT_WALL_TIME:
		/* R11F: the wall clock, or CLOCK_UNAVAILABLE (-1 as int). */
		result = (int) kernel_wall_time();
		break;

	case QT_JOB_SELF:
		/* R11E: the calling job's tagged id. */
		result = (int) job_current_id();
		break;

	case QT_JOB_SLOT:
		/* R11E: tagged id of the job in slot a (live slots only), else -2.
		 * Slot 0 is the system job and is listed like any other. */
		if (a >= JOB_MAX || !jobs[a].in_use) {
			result = -2;
			break;
		}
		result = (int) jobs[a].id;
		break;

	case QT_JOB_RECLAIMED:
		result = (job_lookup(a) == JOB_NONE) ? 1 : 0;
		break;

#ifdef QDOS_JOBCTL
	case QT_JOB_QUERY:
		result = jobctl_query(a, b, c);
		break;

	case QT_JOB_KILL:
		result = jobctl_kill(a);
		break;
#endif

	case QT_CON_WAIT:
		/* b = flag byte.  Input or end already pending: return at once and let
		 * the caller read.  Otherwise sleep indefinitely; the arrival wakes us
		 * and clears the flag.  The trap returns 0 in both cases.  R11E: b must
		 * be a byte in the caller's range; the kernel writes it on arrival. */
		if (b != 0 && !chan_user_range_ok(b, 1)) {
			result = CHAN_ERR_RANGE;
			break;
		}
#ifdef QDOS_VCON
		{
			/* R11M: the wait is on the CALLER'S console.  Input is queued only for the foreground
			 * console, so a job on a background console sleeps until the user switches to it. */
			int cn = vcon_job_con(job_current);
			int w = vcon_waiter(cn);

			if (vcon_count(cn) != 0 || vcon_at_eof(cn)) {
				result = 0;
				break;
			}
			if (w >= 0 && w != job_current && jobs[w].in_use && jobs[w].status < 0) {
				result = CON_ERR_BUSY;
				break;
			}
			vcon_set_waiter(cn, job_current);
			jobs[job_current].status = -1;
			jobs[job_current].hold = (unsigned char *) b;
			f[7] = 0;
			return schedule(sp, 1, 0);
		}
#else
		if (cin_count() != 0 || cin_at_eof()) {
			result = 0;
			break;
		}
		if (con_waiter != JOB_NONE && con_waiter != job_current &&
		    jobs[con_waiter].in_use && jobs[con_waiter].status < 0) {
			result = CON_ERR_BUSY;
			break;
		}
		con_waiter = job_current;
		jobs[job_current].status = -1;
		jobs[job_current].hold = (unsigned char *) b;
		f[7] = 0;
		return schedule(sp, 1, 0);
#endif

#ifdef QDOS_PAGING
	case QT_PG_START:
		result = img_pg_start_request(a, b, c);
#ifdef QDOS_JOBCTL
		if (result > 0)
			jobctl_note_launch(result, a, JC_F_PAGED);
#endif
		break;

	case QT_PG_INFO:
		result = pg_info_fill(a, b);
		break;

	case QT_PG_CAPS:
		result = pg_caps();
		break;

	case QT_PG_CANCEL:
		result = pg_cancel_held(a);
		break;
#ifdef QDOS_TEST_PG_TASKS
	case QT_PG_TEST:
		result = pg_test_value(a);
		break;
#endif
#endif

	default:
		result = -1;
		break;
	}
	f[7] = (unsigned int) result;
	return sp | 1u;                       /* same frame, no scheduling */
}

#ifdef QDOS_PAGING
/* R11J: end the current job from an exception frame (paging.c pg_exception).  The frame
 * at sp is saved but never resumed; the job is reclaimed like any other dead job. */
unsigned int job_kill_current(unsigned int sp)
{
	struct job *j = &jobs[job_current];

	j->prio_inc = 0;
	j->status = -1;
	j->dead = 1;
	return schedule(sp, 1, 0);
}
#endif
