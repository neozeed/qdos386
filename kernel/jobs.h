/*
 * QDOS/386 R1 - native job model, service traps and scheduler interface.
 *
 * The job record is a native i386 structure, not the QDOS job header.
 * Field correspondence is in docs/JS-ROM-KERNEL-MAP.md.
 */
#ifndef QDOS386_JOBS_H
#define QDOS386_JOBS_H

#define JOB_MAX 4                 /* slot 0 SYSTEM, 1..3 application/idle */
#define JOB_STACK_WORDS 1024      /* 4 KiB per job stack */
#define JOB_SYSTEM 0
#define JOB_NONE (-1)

/* Service traps: int 0x30, EAX = function, EBX/ECX/EDX = arguments,
 * EAX = result on return.  Numbers follow the JS ROM trap names. */
#define QT_SUSPEND    0x08        /* a = timeout ticks (<0 indefinite), b = hold byte ptr */
#define QT_REACTIVATE 0x09        /* a = job id */
#define QT_ACTIVATE   0x0A        /* a = job id, b = priority increment */
#define QT_CHPRIO     0x0B        /* a = job id, b = priority increment */
#define QT_INFO       0x0C        /* a = job id -> low byte inc, next byte acc */
#define QT_CREATE     0x0D        /* a = entry, b = inc, c = name -> job id */
#define QT_POLL_ADD   0x0E        /* a = qtask node (poll list) */
#define QT_SCHED_ADD  0x0F        /* a = qtask node (schedule list) */

#define TRAP_VECTOR 0x30

/* Linked task list node: SV.PLIST / SV.SHLST (l00a9e). */
struct qtask {
	struct qtask *next;
	void (*fn)(void);
};

struct job {
	unsigned int saved_sp;        /* JOB.A7 equivalent */
	int saved_fmt;                /* 0 = cooperative frame, 1 = interrupt frame */
	unsigned char prio_inc;       /* JOB.PRIORITY increment, 0 = not active */
	unsigned char acc_prio;       /* JOB accumulated priority */
	int status;                   /* 0 runnable; >0 timeout ticks; <0 indefinite */
	unsigned char *hold;          /* JOB.HOLD release flag, set to 0xFF */
	unsigned int tag;             /* JOB.TAG, part of the job id */
	int in_use;                   /* slot allocated */
	int dead;                     /* job has terminated */
	const char *name;
	void (*entry)(void);
	unsigned int stack_base;
	unsigned int stack_top;
	unsigned long preempts;
	unsigned long dispatches;
};

extern struct job jobs[JOB_MAX];
extern int job_current;
extern int job_idle;                       /* slot of the idle job */
extern volatile unsigned long sched_ticks;          /* timer interrupts handled */
extern volatile unsigned long sched_yields;         /* cooperative yields */
extern volatile unsigned long sched_passes;         /* scheduler passes */
extern volatile unsigned long poll_runs;            /* poll-list task executions */
extern volatile unsigned long sched_runs;           /* schedule-list task executions */
extern volatile unsigned int sv_pollm;              /* SV.POLLM: ticks since last pass */

void sched_init(void);
int job_alloc(const char *name, void (*entry)(void), unsigned char inc);
int job_lookup(unsigned int id);            /* -1 if the id is stale or invalid */
void job_idle_set(int slot);
void job_exit(void);
void job_run(void);
void yield_asm(void);

/* Service trap entry from C.  Defined in kernel/context.s. */
int qtrap(unsigned int fn, unsigned int a, unsigned int b, unsigned int c);

/* Called from kernel/context.s; each returns saved_sp | fmt. */
unsigned int sched_irq_tick(unsigned int sp);
unsigned int sched_yield_sp(unsigned int sp);
unsigned int trap_dispatch(unsigned int sp);

/* Platform hook: end-of-interrupt for IRQ0 (no-op on i386VM). */
void timer_ack(void);

#endif
