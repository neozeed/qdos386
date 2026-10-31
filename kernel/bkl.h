/*
 * QDOS/386 R16 - Big Kernel Lock (docs/R16-ARCHITECTURE.md).  One spinlock serializes all shared-kernel execution.
 * Recursive and owner-checked so the irregular entry boundaries are safe: bkl_enter on a CPU that already owns it
 * only bumps a per-CPU depth; bkl_leave only releases at depth 0 and is a no-op on a CPU that does not own it.
 * Held with interrupts off (interrupt gates); never across an iret to user (resume_dispatch releases first); never
 * while waiting for a device (device IRQs are CPU0's; a blocking syscall schedules another job, which releases).
 * Acquire spins with a locked xchg; no deadlock because the owner always releases without needing the waiter's CPU.
 */
#ifndef QDOS386_BKL_H
#define QDOS386_BKL_H
extern volatile unsigned int bkl_word;              /* 0 free, else owner cpu+1 */
extern int bkl_depth[2];
extern unsigned long bkl_acq[2], bkl_rel[2], bkl_spins[2], bkl_maxspin, bkl_badrel;
unsigned int bkl_xchg(volatile unsigned int *p, unsigned int v);   /* bkl_s.s */
unsigned int bkl_cmpxchg(volatile unsigned int *p, unsigned int expected, unsigned int v);   /* bkl_s.s: owner-safe acquire */
void bkl_pause(void);
int kx_this_cpu(void);
void bkl_enter(void);
void bkl_leave(void);
#endif
