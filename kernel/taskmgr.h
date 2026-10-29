/*
 * QDOS/386 R12C - SYSTASK: global task selector and periodic SYNC (opt-in: -DQDOS_TASKMGR).
 * See kernel/taskmgr.c and docs/R12C-TASK-MANAGER-ABI.md.
 */
#ifndef QDOS386_TASKMGR_H
#define QDOS386_TASKMGR_H

#define QT_TASKMGR     0x3A       /* a = TM_OP_* -> see below */
#define TM_OP_OPEN     0u         /* open the selector (as Ctrl+Esc) -> 0, or -1 before boot finished */
#define TM_OP_STATE    1u         /* -> 1 if the selector is open, else 0 */
#define TM_OP_OPENS    2u         /* -> how many times it was opened */

int taskmgr_start(void);          /* boot: SYSTASK runs in the SYSTEM job -> 0 */
void taskmgr_poll(void);          /* the SYSTEM job's loop, every tick */
int taskmgr_open(void);           /* interrupts off */
int taskmgr_is_open(void);
int taskmgr_trap(unsigned int op);

/* jobs.c: mark a sleeping job runnable now (timed or indefinite sleep); never schedules. */
void job_wake(int slot);

#endif
