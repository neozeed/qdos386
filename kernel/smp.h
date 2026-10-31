/*
 * QDOS/386 R16 - two-CPU scheduling service (docs/R16-ARCHITECTURE.md).  CPU 0 owns all devices; CPU 1 runs
 * CPL3-affine jobs and enters the shared kernel only under the BKL.  QT_SMP is a diagnostic/control trap.
 */
#ifndef QDOS386_SMP_H
#define QDOS386_SMP_H
#define QT_SMP          0x3E
#define SMP_OP_INFO     0        /* c = 24-word buffer -> 0 */
#define SMP_OP_SETAFF   1        /* b = job id, c = cpu (0/1) -> 0 or negative; protected jobs only may go to CPU 1 */
#define SMP_OP_START    2        /* bring up CPU 1 now (if not already); -> state */
#define SMP_INFO_WORDS  24
#define SMP_ERR_UNAVAIL (-90)
#define SMP_ERR_ARG     (-91)
#define SMP_ERR_RING0   (-92)    /* a ring-0 job cannot be placed on CPU 1 */
#define SMP_ERR_STATE   (-93)
#define SMP_S_DOWN      0
#define SMP_S_UP        1
#define SMP_S_FAILED    2
void smp_boot(void);             /* pre-paging: detect CPUs, calibrate */
void smp_start_ap(void);         /* post-paging/timer: INIT/SIPI the AP into the scheduler */
int smp_trap(unsigned int a, unsigned int b, unsigned int c);
int smp_cpu_count(void);
int smp_affinity(int slot);      /* 0 or 1; CPU0 for every job until SETAFF */
void smp_ap_timer_tick(void);
#endif
