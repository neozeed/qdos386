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
#ifdef QDOS_R16B
/* R16B (docs/R16B-MIGRATION-ABI.md).  SETAFF gains c = SMP_CPU_AUTO (clear a manual pin).  STATS is read-only and
 * versioned: b = caller buffer size in words, c = buffer; returns the number of words written (or an error). */
#define SMP_OP_STATS    3        /* b = words, c = buffer -> words written */
#define SMP_OP_POLICY   4        /* b = SMP_POL_* -> previous policy */
#ifdef QDOS_R17
/* R17 Apollo 13 (docs/APOLLO13-TOPOLOGY.md): up to 8 CPUs.  SETAFF c = 0..7 or SMP_CPU_AUTO8; STATS version 2. */
#define SMP_OP_TOPO     6            /* b = words, c = buffer -> words written: topology table */
#define SMP_OP_HT       7            /* b = SMP_HT_* or SMP_POL_QUERY -> previous mode, or negative */
#define SMP_HT_ALL      0u           /* every online logical CPU may run background work */
#define SMP_HT_AUTO     1u           /* distinct physical cores first, then HT siblings (CPU 0's sibling last) */
#define SMP_HT_PHYSICAL 2u           /* HT siblings parked: one logical CPU per physical core */
#define SMP_CPU_AUTO8   8u           /* R17 SETAFF c: no manual pin (R16B's 2 is CPU 2 here) */
#define SMP_ERR_PARKED  (-96)        /* that CPU is offline or parked */
#define SMP_ERR_NOTOPO  (-97)        /* that HT mode needs a proven topology with SMT siblings */
#define SMP_STATS_VERSION2 2u
#define SMP_STATS_WORDS2 144u
#define SMP_TOPO_WORDS  96u
#define SMP_OP_MEMINFO  5            /* R17: b = words, c = buffer -> words written (docs/APOLLO13-MEMORY-MAP.md) */
int r17_cpu_parked(int cpu);         /* smp17.c */
#define SMP_MEMINFO_WORDS 192
int proc_meminfo(unsigned int words, unsigned int buf);   /* proc.c */
#endif
#define SMP_CPU_AUTO    2        /* SETAFF c: no manual pin; the policy (or CPU 0) decides */
#define SMP_POL_QUERY   0xFFu    /* POLICY b: report only */
#define SMP_POL_OFF     0u       /* every AUTO job prefers CPU 0 (R16A behaviour) */
#define SMP_POL_FG      1u       /* foreground-first: AUTO protected jobs of the foreground session prefer CPU 0, others CPU 1 */
#define SMP_POL_LEND    2u       /* FG, plus an otherwise idle CPU 0 may borrow READY background jobs (work-conserving) */
#define SMP_ERR_NOJOB   (-94)    /* no live job with that id */
#define SMP_ERR_DEAD    (-95)    /* the job is terminating */
#define SMP_STATS_VERSION 1u
#define SMP_STATS_WORDS 48u
void smp_acct_tick(unsigned int sp);          /* either CPU's timer tick, BKL held: sample what was interrupted */
void smp_note_dispatch(int slot, int cpu);    /* schedule(): a switch to slot on cpu (context switch / migration) */
void smp_policy_tick(void);                   /* CPU 0 tick: apply the foreground policy at a safe point */
#ifndef R16B_LEND_RES
#define R16B_LEND_RES 50u                     /* ticks CPU 1 leaves a job lent to an idle CPU 0 alone */
#endif
int smp_steal(int cpu);                       /* pick_next on an otherwise idle CPU 0: a READY background job, or -1 */
extern volatile unsigned long smp_focus_gen;  /* bumped by vcon_switch on every foreground change */
#endif
void smp_boot(void);             /* pre-paging: detect CPUs, calibrate */
void smp_start_ap(void);         /* post-paging/timer: INIT/SIPI the AP into the scheduler */
int smp_trap(unsigned int a, unsigned int b, unsigned int c);
int smp_cpu_count(void);
int smp_affinity(int slot);      /* 0 or 1; CPU0 for every job until SETAFF */
void smp_ap_timer_tick(void);
#endif
