/*
 * QDOS/386 R4 - native job model, service traps and scheduler interface.
 *
 * R3 changes from R2:
 *   - job stacks come from the kernel heap (kernel/memory.c), not a static
 *     array; each stack has a canary at its base and a high-water mark;
 *   - every job owns the heap blocks it allocates through QT_MEM_ALLOC, and
 *     they are reclaimed with the job;
 *   - deferred reclamation runs only in trap and cooperative-yield context,
 *     never in the timer ISR.
 *
 * The job record is a native i386 structure, not the QDOS job header.
 * Field correspondence is in docs/JS-ROM-KERNEL-MAP.md.
 */
#ifndef QDOS386_JOBS_H
#define QDOS386_JOBS_H

#include "fpu.h"

#ifdef QDOS_PAGING_POC
#error "R11K: the paging feature macro is now QDOS_PAGING (ABI 2); QDOS_PAGING_POC was the R11J name"
#endif
#ifdef QDOS_DYNPROC
#define JOB_MAX 16                /* R12A: a bounded table; job memory (frames, kernel stacks) is dynamic */
#else
#define JOB_MAX 6                 /* R10: slot 0 SYSTEM, IDLE, shell, and room for two
                                  * floating-point application jobs beside them */
#endif
#define JOB_STACK_BYTES 4096      /* heap-allocated, 16-byte aligned */
#define JOB_STACK_CANARY 0xc0dec0deu
#define JOB_SYSTEM 0
#define JOB_NONE (-1)

/* Service traps: int 0x30, EAX = function, EBX/ECX/EDX = arguments,
 * EAX = result on return.  R2 numbers follow the JS ROM trap names.
 * R3 memory functions use 0x10..0x13, R4 channel services 0x14..0x17. */
#define QT_SUSPEND    0x08        /* a = timeout ticks (<0 indefinite), b = hold byte ptr */
#define QT_REACTIVATE 0x09        /* a = job id */
#define QT_ACTIVATE   0x0A        /* a = job id, b = priority increment */
#define QT_CHPRIO     0x0B        /* a = job id, b = priority increment */
#define QT_INFO       0x0C        /* a = job id -> low byte inc, next byte acc */
#define QT_CREATE     0x0D        /* a = entry, b = inc, c = name -> job id */
#define QT_POLL_ADD   0x0E        /* a = qtask node (poll list) */
#define QT_SCHED_ADD  0x0F        /* a = qtask node (schedule list) */
#define QT_MEM_ALLOC  0x10        /* a = bytes -> pointer, or negative MEM_ERR_* */
#define QT_MEM_FREE   0x11        /* a = pointer -> 0, or negative MEM_ERR_* */
#define QT_MEM_FREE_BYTES 0x12    /* -> free bytes in the heap */
#define QT_MEM_VALIDATE   0x13    /* -> 0 if heap invariants hold, else MEM_ERR_CORRUPT */
/* R4 channel services (kernel/channel.c).  EAX = code; ids are (tag<<16)|index. */
#define QT_CH_OPEN    0x14        /* a = name pointer ("CON", NUL-terminated) -> channel id, or negative */
#define QT_CH_CLOSE   0x15        /* a = channel id -> 0, or negative */
#define QT_CH_WRITE   0x16        /* a = channel id, b = buffer, c = length -> bytes written */
#define QT_CH_PUTC    0x17        /* a = channel id, b = byte -> 1, or negative */
/* R5 application services.  New numbers only; 0x08..0x17 are unchanged. */
#define QT_EXIT       0x18        /* a = exit status, recorded for the calling job -> 0 */
#define QT_JOB_EXIT   0x19        /* a = job id -> exit status, or negative (see JOB_ERR_*) */
/* R6 file channels (WIN1_ names; see docs/R6-ABI.md). */
#define QT_CH_READ    0x1A        /* a = channel id, b = buffer, c = length (<= 256) -> bytes read, 0 at end */
#define QT_CH_SEEK    0x1B        /* a = channel id, b = byte position -> 0, or negative */
#ifdef QDOS_FS_WRITE
#define QT_CH_OPENX   0x2A        /* R11L: a = name, b = CHAN_O_* flags -> channel id, or negative */
#define QT_CH_CTL     0x2B        /* R11L: a = channel id, b = CHAN_CTL_*, c = argument -> result */
#define QT_FS_OP      0x2C        /* R11L: a = CHAN_FS_*, b = name, c = second name -> result */
#endif
#define QT_IMG_LOAD   0x1D        /* a = name pointer (WIN1_ root name) -> job id, or negative */
#define QT_IMG_START  0x24        /* R11H: a = name pointer, b = profile 0..3 or -1 for any -> tagged job id, or negative */
#define QT_JOB_RECLAIMED 0x1E     /* a = job id -> 1 if the job slot is free again, else 0 */
/* R8: application arguments and time. */
#define QT_APP_ARGS   0x1F        /* a = pointer to a word -> argc; the word gets the argv array address (0 if none) */
#define QT_TICKS      0x20        /* -> sched_ticks, low 32 bits (timer ticks since boot) */
#define QT_JOB_SELF   0x21        /* R11E: -> tagged job id of the calling job */
#define QT_JOB_SLOT   0x22        /* R11E: a = slot index -> tagged job id of the job in that slot, or -2 */
#define QT_WALL_TIME  0x23        /* R11F: -> wall time (epoch seconds), or -1 (0xFFFFFFFF) if unavailable */
#ifdef QDOS_DYNPROC
/* R12A process services (docs/R12A-PROCESS-ABI.md). */
#define QT_PROC_CAPS  0x2F        /* -> (ABI version << 24) | (job table size << 16) | PROC_CAP_* */
#define QT_PROC_SPAWN 0x30        /* a = name, b = struct qd_spawn pointer -> tagged job id */
#define QT_PROC_WAIT  0x31        /* a = child id, b = timeout ticks (-1 forever, 0 poll) -> exit status, or negative */
#define QT_PROC_INFO  0x32        /* a = job id (0 = caller), b = PROC_INFO_WORDS-word buffer -> 0 */
#define QT_PROC_KILL  0x33        /* a = child id -> 0 */
#define QT_PROC_VA    0x34        /* a = virtual address -> its PTE in the caller's space (diagnostic) */
extern unsigned int kx_lo, kx_hi, kx_entry;   /* the current job's kernel-entry stack (kctx.s reads them) */
#endif
#ifdef QDOS_ENV
/* R12B per-process environment (docs/R12B-ENV-PROCESS-ABI.md). */
#define QT_ENV_GET    0x35        /* a = name, b = buffer, c = size -> value length (copy truncated to size-1), -7 absent */
#define QT_ENV_SET    0x36        /* a = name, b = value (0 = unset) -> 0, or -15 name, -4 value, -5 full, -3 memory, -7 */
#define QT_ENV_LIST   0x37        /* a = index (0xFFFFFFFF: (count << 16) | bytes), b = buffer, c = size -> length, -7 end */
#endif
#define QT_CON_WAIT   0x1C        /* b = flag byte, set when console input arrives -> 0.
                                     Returns at once if input or end is pending; otherwise
                                     the job sleeps.  The caller retries its CON read. */

/* QT_JOB_EXIT errors.  -2 is the existing "no such job" code (QT_INFO). */
#define JOB_ERR_RUNNING  (-16)    /* job exists and has not ended */
#define JOB_ERR_NOSTATUS (-17)    /* job ended without calling QT_EXIT */
#define CON_ERR_BUSY    (-50)    /* QT_CON_WAIT: another job is already waiting */

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
	unsigned int id;              /* (tag << 16) | slot; also the heap owner */
	int in_use;                   /* slot allocated */
	int dead;                     /* job has terminated */
	const char *name;
	void (*entry)(void);
	unsigned int stack_base;      /* heap block; 0 for the boot stack */
	unsigned int stack_top;
	unsigned int stack_hwm;       /* deepest stack use seen at reclaim, bytes */
	unsigned int stack_bytes;     /* R8: stack size of this job */
	unsigned int args;            /* R8: argument block address in the slot, 0 if none */
	unsigned long preempts;
	unsigned long dispatches;
	unsigned int fpu[FPU_STATE_WORDS];   /* R10: x87 state (FNSAVE image) */
#ifdef QDOS_DYNPROC
	unsigned int kx_lo, kx_hi;    /* R12A: kernel stack range; an entry from outside it switches to kx_entry */
	unsigned int kx_entry;        /* R12A: kernel-stack ESP for entries from the private user stack */
#endif
};

/* Exit record per slot.  Kept after the job is reclaimed, so SYSTEM can read
 * the status later.  It is replaced when the slot is reused (new id). */
struct job_exit_rec {
	unsigned int id;              /* job id that owns this record; 0 = none */
	int exited;                   /* QT_EXIT was called */
	unsigned int status;          /* value passed to QT_EXIT */
};

extern struct job jobs[JOB_MAX];
extern int job_current;
#ifdef QDOS_R13B
/* R13B: a kernel launch on behalf of another job (SYSTASK F2 with the boot shell's directory and PATH): while
 * job_ctx_override >= 0, the program search, the new job's directory and its environment come from that slot instead
 * of the current job.  Set and cleared with interrupts off around one load. */
extern int job_ctx_override;
#define JOB_CTX_SLOT() (job_ctx_override >= 0 ? job_ctx_override : job_current)
#else
#define JOB_CTX_SLOT() (job_current)
#endif
extern int job_idle;                       /* slot of the idle job */
extern volatile unsigned long sched_ticks;          /* timer interrupts handled */
extern volatile unsigned long sched_yields;         /* cooperative yields */
extern volatile unsigned long sched_passes;         /* scheduler passes */
extern volatile unsigned long poll_runs;            /* poll-list task executions */
extern volatile unsigned long sched_runs;           /* schedule-list task executions */
extern volatile unsigned int sv_pollm;              /* SV.POLLM: ticks since last pass */
extern volatile unsigned long stack_faults;         /* canary failures seen in the ISR */
extern unsigned long jobs_reclaimed;                /* jobs whose resources were freed */
extern unsigned long bytes_reclaimed;               /* heap bytes freed by reclaim */

void sched_init(void);
int job_alloc(const char *name, void (*entry)(void), unsigned char inc);
/* R8: as job_alloc, with a stack of stack_bytes (16-byte multiple, 4 KiB..64 KiB). */
int job_alloc_sized(const char *name, void (*entry)(void), unsigned char inc, unsigned int stack_bytes);
void job_cancel(unsigned int id);            /* R8: free a job that never ran */
int job_lookup(unsigned int id);            /* -1 if the id is stale or invalid */
unsigned int job_current_id(void);
int stack_canary_ok(int slot);
void job_idle_set(int slot);
void job_exit(void);
void job_run(void);
void yield_asm(void);

/* Service trap entry from C.  Defined in kernel/context.s. */
int qtrap(unsigned int fn, unsigned int a, unsigned int b, unsigned int c);

/* How a C entry point hands the frame to the resume path (KCTX-FIX2, docs/KCTX-FIX2-IMPLEMENTATION.md).
 * Default (kernel/kctx.s, every current kernel): the FULL stack pointer is returned and the frame format goes in
 * kx_resume_fmt, written with IF clear just before the return and read by resume_dispatch.  QDOS_KCTX_PTRBITS (set by
 * the Makefile only for targets that link context.s, paging.s or kernel/frozen/kctx-r12a-r14.s, i.e. the frozen
 * R6..R14 images): the historical value saved_sp | fmt, decoded there with a mask that assumes a 4-byte-aligned stack. */
#ifdef QDOS_KCTX_PTRBITS
#define KX_RESUME(sp, fmt) ((sp) | (unsigned int) (fmt))
#elif defined(QDOS_KCTX_CHECK)
/* Test-only kernels (make qdos386-check-pc): the same hand-off through a checking function (jobs.c). */
extern unsigned int kx_resume_fmt;
unsigned int kx_resume_check(unsigned int sp, unsigned int fmt);
#define KX_RESUME(sp, fmt) kx_resume_check((unsigned int) (sp), (unsigned int) (fmt))
#else
extern unsigned int kx_resume_fmt;
#define KX_RESUME(sp, fmt) (kx_resume_fmt = (unsigned int) (fmt), (unsigned int) (sp))
#endif

/* Called from kernel/context.s; each returns KX_RESUME(saved_sp, fmt). */
unsigned int sched_irq_tick(unsigned int sp);
unsigned int sched_yield_sp(unsigned int sp);
unsigned int trap_dispatch(unsigned int sp);

/* Platform hook: end-of-interrupt for IRQ0 (no-op on i386VM). */
void timer_ack(void);

#endif
