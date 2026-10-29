/*
 * QDOS/386 R12A - dynamic process address spaces (QDOS_DYNPROC kernels).  See docs/R12A-PROCESS-ABI.md and
 * docs/R12A-MEMORY-MAP.md.
 *
 * Every application is a process: its own page directory, image pages from the frame pool at the image's link origin,
 * a private heap and a private user stack.  The job keeps a kernel stack in the shared kernel heap; every kernel entry
 * runs on it (kctx.s).  The R11J/R11K paging entry points (pg_*, paging.h) are provided by proc.c with the same
 * meaning, so jobs.c, channel.c and imgload.c keep their hooks.
 *
 * Privilege: everything still runs at CPL 0.  Private memory is a correctness and ownership property, NOT isolation.
 */
#ifndef QDOS386_PROC_H
#define QDOS386_PROC_H

#include "vas.h"
#include "pheap.h"
#include "aout.h"

#define PROC_HEAP_BASE    0x10000000u
#ifndef QDOS_PROC_HEAP_MAX
#define QDOS_PROC_HEAP_MAX 0x00800000u          /* 8 MiB of virtual heap per process; frames are the real limit */
#endif
#define PROC_HEAP_MAX     QDOS_PROC_HEAP_MAX
#define PROC_STACK_TOP    0x20000000u
#ifndef QDOS_PROC_STACK
#define QDOS_PROC_STACK   0x00010000u           /* 64 KiB user stack, committed at creation */
#endif
#define PROC_STACK_BYTES  QDOS_PROC_STACK
#define PROC_KSTACK_BYTES 16384u                /* kernel-entry stack, shared kernel heap */

/* R12A process ABI version (QT_PROC_CAPS bits 24..31) and capability bits. */
#define PROC_ABI_VERSION  1
#define PROC_CAP_DYNAMIC  0x0001u   /* no fixed slots: same-origin images get private frames */
#define PROC_CAP_PHEAP    0x0002u   /* QT_MEM_ALLOC from a process uses its private heap */
#define PROC_CAP_PSTACK   0x0004u   /* private user stack with a guard; kernel entries on a kernel stack */
#define PROC_CAP_SPAWN    0x0008u   /* QT_PROC_SPAWN/WAIT/KILL from any process */
#define PROC_CAP_GUARD    0x0010u   /* user-stack overflow ends only the process (#DF task gate) */

/* Errors (negative; -60.. shared with the R11J/R11K paging codes). */
#define PROC_ERR_NOFRAMES (-60)     /* frame pool exhausted */
#define PROC_ERR_ORIGIN   (-61)     /* image origin not usable as a private window */
#define PROC_ERR_REFUSED  (-62)     /* operation not available to a process */
#define PROC_ERR_NOTCHILD (-68)     /* QT_PROC_WAIT/KILL: not a child of the caller */
#define PROC_ERR_LIMIT    (-69)     /* child limit or job table full */
#define PROC_ERR_TIMEOUT  (-70)     /* QT_PROC_WAIT: timed out (poll: still running) */
#define PROC_ERR_AGAIN    (-73)     /* QT_PROC_WAIT: woken, repeat the call (libqdos does) */
#define PROC_CHILD_LIMIT  8         /* unreaped children per parent */

/* QT_PROC_INFO result. */
#define PROC_INFO_WORDS   24

/* Boot: template, paging on, RAM measured, frame pool built and tested.  Prints one line. */
void proc_boot(void);

/* Load an a.out from any reader into a new process.  want_slot: the historical profile the caller requires (0..3) or
 * -1 for any origin, with the R8/R11H error codes for a mismatch.  flags: PROC_LOAD_HELD (created inactive).  Returns
 * the tagged job id or a negative code; on failure nothing is left behind. */
#define PROC_LOAD_HELD    0x0001u
int proc_load(app_rd_fn rd, void *ctx, unsigned int len, int want_slot, const unsigned char *packed,
              unsigned int plen, unsigned int flags);
int proc_shell_is_process(void);

/* Trap bodies used by jobs.c. */
int proc_mem_alloc(unsigned int bytes);
int proc_mem_free(unsigned int ptr);
int proc_mem_validate(void);
int proc_caps(void);
int proc_info(unsigned int id, unsigned int user_addr);
int proc_va(unsigned int va);
unsigned int proc_df_kill(unsigned int sp);
int proc_spawn(unsigned int name_addr, unsigned int req_addr);
int proc_wait(unsigned int id, int timeout, int *block);
int proc_kill(unsigned int id);
void proc_job_reaped(unsigned int id, int status);

#endif
