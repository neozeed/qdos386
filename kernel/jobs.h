/*
 * QDOS/386 R1 - native job model and scheduler interface.
 *
 * The job record is a native i386 structure.  It is not the historical
 * QDOS job header layout.  The mapping is in docs/JS-ROM-KERNEL-MAP.md.
 */
#ifndef QDOS386_JOBS_H
#define QDOS386_JOBS_H

#define JOB_MAX 4
#define JOB_STACK_WORDS 1024          /* 4 KiB per job stack */

#define JOB_RUN 0                     /* runnable */
#define JOB_SUSPENDED 1               /* suspended indefinitely */
#define JOB_DEAD 2                    /* terminated */

#define JOB_IDLE_INDEX 3              /* idle job is always the last slot */

struct job {
	unsigned int saved_sp;            /* saved stack pointer (format in saved_fmt) */
	int saved_fmt;                    /* 0 = CTX (cooperative), 1 = IRQ (preempted) */
	unsigned char prio_inc;           /* QDOS JOB.PRIORITY increment; 0 = never runs */
	unsigned char acc_prio;           /* QDOS JOB.ACCUMULATED priority */
	int status;                       /* JOB_RUN, JOB_SUSPENDED or JOB_DEAD */
	const char *name;
	void (*entry)(void);
	unsigned int stack_base;          /* lowest stack address (static pool) */
	unsigned int stack_top;           /* initial ESP is stack_top */
	unsigned long preempts;           /* times this job was preempted by a timer tick */
	unsigned long dispatches;         /* times this job was dispatched */
};

extern struct job jobs[JOB_MAX];
extern int job_current;
extern unsigned long sched_ticks;        /* timer ticks observed by the scheduler */
extern unsigned long sched_yields;       /* cooperative yields observed */

void sched_init(void);
int job_create(const char *name, void (*entry)(void), unsigned char prio_inc,
               unsigned int stack_base, unsigned int stack_bytes);
void job_register_boot(const char *name, unsigned char prio_inc);
void job_suspend_self(void);
void job_exit(void);
void job_run(void);
void yield_asm(void);

/* Called from kernel/context.s with the saved-state pointer. Return saved_sp|fmt. */
unsigned int sched_irq_tick(unsigned int sp);
unsigned int sched_yield_sp(unsigned int sp);

#endif
