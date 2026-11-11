/*
 * QDOS/386 R20 (Apollo 16) - the Ring 0 storage worker, its request table and the bounded sector cache.
 *
 * Opt-in only (QDOS_R20).  Not a public API: the trap numbers below are internal and exist for the R20 tests (tests/a16).
 * The worker is an ordinary Ring 0 kernel job (job_alloc, its own supervisor stack, scheduled by the existing scheduler).
 * Its body runs WITHOUT the BKL; every transition of shared state happens inside a trap (under the BKL, interrupts off),
 * so completion-before-sleep and wakeup-before-sleep cannot be lost.
 *
 * Phase 1 (internal request table):
 *   R20_SUBMIT(op, arg) -> ticket (>0) or R20_ERR_FULL.  Never sleeps.
 *   R20_WAIT(ticket)    -> the result when DONE; otherwise the caller sleeps and the trap returns R20_RETRY.
 *   R20_TAKE()          -> worker only: a ticket for the oldest pending work, or 0 after sleeping while nothing is pending.
 *   R20_INFO(ticket)    -> worker only: op and argument of an ACTIVE user ticket it owns.
 *   R20_DONE(ticket, r) -> worker only: publish the result, wake the waiter, release abandoned slots.
 *   R20_STAT(index)     -> diagnostic counter (R20_STAT_*).
 *
 * Phase 2A (restartable cache miss): a file read (QT_CH_READ on a file channel) is PROBED on a copy of the channel state
 * into a kernel buffer.  A missing sector puts the caller to sleep on a cache entry the worker fills, and the trap rewinds
 * the saved EIP over its own "int 0x30", so the same call runs again on wake.  Only a complete probe commits.
 *
 * Phase 2B (bounded sector cache, kernel/r20w.c):
 *   - Budget: min(5% of firmware-reported usable RAM, 2 MiB), counting data AND metadata (entries, buckets, queue).
 *   - Keyed by device identity (one IDE slave: dev 0) and physical LBA (the partition offset is applied before the cache).
 *   - Hashed lookup, clock (second-chance) eviction, per-entry pins held by the worker, generation numbers.
 *   - Simultaneous misses to one sector share one fill (coalesced).  LOADING and pinned entries are never evicted.
 *   - All sector writes invalidate the sector before and after the write (writes stay synchronous).
 *   - No lock, and no BKL, is held while the worker polls the disk.
 */
#ifndef QDOS386_R20W_H
#define QDOS386_R20W_H

#ifdef QDOS_R20

#define QT_R20_SUBMIT 0x40
#define QT_R20_WAIT   0x41
#define QT_R20_TAKE   0x42
#define QT_R20_INFO   0x43
#define QT_R20_DONE   0x44
#define QT_R20_STAT   0x45

#define R20_NQ        8           /* fixed user request table */
#define R20_OP_SPIN   1           /* synthetic bounded CPU work: arg iterations, result (3*arg+1) & 0x7fffffff */
#define R20_SPIN_MAX  4000000u

#define R20_CACHE_BIT    (1u << 30)  /* a TAKE/DONE ticket with this bit is a cache fill, not a user request */
#define R20C_IDX_BITS    12          /* cache ticket: bit 30 | generation (18 bits, 12..29) | entry index (12 bits) */
#define R20C_MAX_ENT     4096u       /* index limit; the 2 MiB budget gives about 3,900 entries */
#define R20C_MIN_ENT     8u          /* below this the cache is disabled and file reads run synchronously */
#define R20C_BUDGET_MAX  (2u * 1024u * 1024u)
#define R20C_PCT         5u          /* percent of usable RAM */
#define R20C_SECTOR      512u
#define R20C_OWNER       0x7E20u     /* heap owner of the cache blocks (never a job id) */
#ifndef R20_RSV_MAX
#define R20_RSV_MAX 4             /* completed sectors one reader may hold reserved at once (a 256-byte read needs at most 3) */
#endif
#define R20_MAX_MISS  64          /* restarts one process may take in a row before its read fails (bounded) */
#define R20_DEV_SPIN  100000000ul /* bounded wait for the storage claim (a sync trap polls with the BKL held) */

#define R20_ERR_FULL    (-40)
#define R20_ERR_STALE   (-41)
#define R20_ERR_OWNER   (-42)
#define R20_ERR_ARG     (-43)
#define R20_ERR_NOWORK  (-44)
#define R20_RETRY       (-45)
#define R20_RESTART     (-101)    /* a file read must be re-executed after the cache fill (kernel-internal) */

enum r20_worker_state { R20W_STOPPED = 0, R20W_READY, R20W_RUNNING, R20W_BLOCKED };

/* Indices 0..23 are the Phase 1 and 2A counters, unchanged (the tests read them by number). */
enum r20_stat {
	R20_STAT_SUBMITS = 0, R20_STAT_FULL, R20_STAT_WAIT_DONE, R20_STAT_WAIT_SLEPT, R20_STAT_WAKE_WORKER,
	R20_STAT_TAKES, R20_STAT_WORKER_SLEPT, R20_STAT_COMPLETIONS, R20_STAT_WAKE_SUBMITTER, R20_STAT_ABANDONED,
	R20_STAT_GONE_FREED, R20_STAT_STALE, R20_STAT_STATE,
	R20_STAT_HIT, R20_STAT_MISS, R20_STAT_FILL, R20_STAT_FILL_OK, R20_STAT_FILL_ERR, R20_STAT_DEV_WAIT,
	R20_STAT_INVALIDATE, R20_STAT_GAVE_UP, R20_STAT_FILL_DISCARDED, R20_STAT_WAKE_CACHE, R20_STAT_RESTART,
	/* Phase 2B */
	R20_STAT_COALESCED, R20_STAT_EVICT, R20_STAT_NOSPACE, R20_STAT_IDE_READ, R20_STAT_IDE_WRITE,
	R20_STAT_SYNC_READ, R20_STAT_CACHE_ENTRIES, R20_STAT_CACHE_BYTES, R20_STAT_CACHE_USED, R20_STAT_INVAL_ALL,
	R20_STAT_MY_GIVEUP,   /* GAVE_UP accounting for the calling job only (a stat call from a reader) */
	R20_STAT_DUMP,        /* writes a state dump to the console (entries, waiters, queue); returns the entry count */
	R20_STAT_RSV_SET,     /* a completed fill reserved for a waiting reader */
	R20_STAT_RSV_RELEASE, /* a reservation released (commit, error, process end) */
	R20_STAT_RSV_FULL,    /* a reader already holds R20_RSV_MAX reservations: the read goes direct, uncached */
	R20_STAT_SYNC_FALLBACK, /* a probe read read one sector directly (no cache entry): reader holds reservations and no entry is free */
	R20_STAT_N
};

/* Boot: the cache blocks (after mem_init, before the mount: the mount reads through the cache). */
void r20c_init(void);
/* Boot: create the worker job (after the APs are up, so the background CPU is known). */
int r20w_start(void);
/* Trap entry from trap_dispatch.  Returns the result; *sleep is set when the caller must sleep (schedule). */
int r20w_trap(unsigned int fn, unsigned int a, unsigned int b, int caller_job, int *sleep);
/* Reclaim hook: a job slot is being freed.  Abandons or releases its requests and its cache waits. */
void r20w_job_gone(int slot, unsigned int id);
/* Is this job slot the storage worker?  (jobs.c: the worker's EFLAGS.IF is managed around its claims.) */
int r20w_is_worker(int slot);
/* Diagnostics. */
unsigned long r20w_stat(int idx);

/* Phase 2A/2B: called by the file layer and by the disk driver (platform/pc/disk.c). */
struct fat16_file;
int r20_file_read(struct fat16_file *ff, unsigned char *ubuf, unsigned int n);
int r20c_read(unsigned int lba, unsigned char *buf);          /* probe: hit or miss; sync: hit, or disk read */
int r20c_write(unsigned int lba, const unsigned char *buf);   /* waits for the claim, invalidates, writes */
int r20c_wait_dev(void);                                      /* 0 when the storage claim is free */
void r20c_invalidate_all(void);                               /* mount / media change: nothing cached is trusted */
int disk_read_worker(unsigned int lba, unsigned char *buf);   /* the worker's raw read (platform/pc/disk.c) */
int disk_read_body(unsigned int lba, unsigned char *buf);
int disk_write_body(unsigned int lba, const unsigned char *buf);
unsigned long disk_sector_count_body(void);

#endif
#endif
