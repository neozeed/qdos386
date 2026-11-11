/*
 * QDOS/386 R20 (Apollo 16) - the Ring 0 storage worker, its request table and the bounded sector cache.
 * See kernel/r20w.h for the protocol and its limits.
 *
 * Locking: all state below changes only inside a trap (under the BKL, interrupts off), except:
 *   - r20_claim is cleared by the worker's body, which runs with the BKL released and interrupts off; sync traps spin on it
 *     WITH the BKL held, which is safe because the worker clears the claim before it needs the BKL (its DONE trap);
 *   - the worker's body reads its own pinned cache entry (lba) and writes its own data buffer.  A pinned entry is never
 *     reassigned, so nothing else touches that buffer until DONE.
 *
 * Cache invariants:
 *   - an entry is FREE, LOADING (queued or taken by the worker), VALID or ERROR;
 *   - a LOADING entry is never a victim; a pinned entry (taken by the worker) is never a victim;
 *   - an entry is in the hash chain exactly when hashed == 1; invalidation unhashes it and bumps its generation, so a
 *     fill that completes later finds a different generation and publishes nothing ("orphan"), and the orphan is freed
 *     when its last reference (queue ticket or worker pin) goes;
 *   - each entry has at most one ticket in the fill queue; the queue has room for every entry, so it never overflows.
 */
#include "jobs.h"
#include "r20w.h"
#include "fat16.h"
#include "channel.h"
#include "memmap.h"
#include "memory.h"
#include "platform.h"

#ifdef QDOS_R20

extern int smp_cpu_count(void);

#define RQ_FREE      0
#define RQ_QUEUED    1
#define RQ_ACTIVE    2
#define RQ_DONE      3
#define RQ_ABANDONED 4

#define CE_FREE    0
#define CE_LOADING 1
#define CE_VALID   2
#define CE_ERROR   3

/* forward declarations: the cache section below is used by the trap code above it */
int r20c_queue_head(void);
void r20c_queue_pop(void);
int r20c_fill_done(unsigned int ticket, int res);
unsigned long r20c_used(void);

#define GEN_MASK   0x3ffffu   /* generation: 18 bits, so it fits the cache ticket (bits 12..29) and the user ticket */
#define NOSPACE_TAG 0xffffu   /* jwait value: asleep for a free cache entry, not for one fill */

struct r20_req {
	unsigned char state;          /* RQ_* */
	unsigned char op;
	unsigned char waiting;        /* the submitter is asleep in R20_WAIT for this request */
	unsigned int gen;
	unsigned int seq;
	unsigned int arg;
	unsigned int submitter_id;    /* tagged job id of the submitter */
	unsigned int waiter_id;
	int waiter_slot;
	int result;
};

struct r20_ce {
	unsigned int lba;
	unsigned int gen;
	unsigned int rmask;           /* readers holding this VALID sector reserved: bit = job slot (not evictable) */
	unsigned int seq;
	unsigned short hnext;         /* hash chain: next entry index + 1, 0 = end */
	unsigned char state;          /* CE_* */
	unsigned char queued;         /* 1 while its ticket waits in the fill queue */
	unsigned char pins;           /* 1 while the worker holds it */
	unsigned char ref;            /* clock reference bit */
	unsigned char hashed;         /* 1 while in the hash chain */
	unsigned char dev;            /* device identity: 0 = the single IDE slave */
};

static struct r20_req rq[R20_NQ];
static int wslot = -1;                       /* the worker's job slot */
static int wstate = R20W_STOPPED;
static unsigned int gen_next = 1;
static unsigned int seq_next = 1;
static unsigned long st[R20_STAT_N];
static unsigned short jwait[JOB_MAX];        /* 0 none; entry index + 1; NOSPACE_TAG */
static unsigned int jwait_id[JOB_MAX];       /* tagged job id the wait belongs to (slot reuse) */
static unsigned char misses[JOB_MAX];        /* restarts taken by this job in a row */
static unsigned int giveup_job[JOB_MAX];     /* GAVE_UP events per job (diagnostics; R20_STAT_MY_GIVEUP) */
static unsigned long inj_n = 0;              /* test-only fault injection counter (R20C_INJECT_EVERY) */
static int r20c_dump(void);                  /* console state dump (R20_STAT_DUMP) */
static volatile int r20_claim = 0;           /* the worker holds the controller: sync traps must wait */
#if JOB_MAX > 32
#error "r20w: rmask holds one bit per job slot"
#endif
static unsigned short rsv_e[JOB_MAX][R20_RSV_MAX];   /* a reader's reserved entries (index) ... */
static unsigned int rsv_g[JOB_MAX][R20_RSV_MAX];     /* ... and their generations (stale ones are skipped) */
static unsigned char rsv_n[JOB_MAX];
static int r20_probe = 0;                    /* 1 while a file read is probed: cache misses return FAT_ERR_MISS */
static unsigned int r20_miss_lba = 0;

/* the cache: allocated once at boot from the heap, sized by usable RAM */
static int c_on = 0;
static unsigned int nent = 0, nb = 0, hand = 0;
static struct r20_ce *ce = 0;
static unsigned char *cdata = 0;             /* nent * 512 */
static unsigned short *bucket = 0;           /* nb */
static unsigned int *cq = 0;                 /* fill queue ring: (gen << 12) | index */
static unsigned int cq_head = 0, cq_count = 0;

static unsigned int next_gen(void)
{
	unsigned int g = gen_next;
	gen_next = (gen_next >= GEN_MASK) ? 1u : gen_next + 1u;
	return g;
}

static unsigned int handle_of(int idx)
{
	return (rq[idx].gen << 8) | (unsigned int) idx;
}

static void job_wake(int slot)
{
	jobs[slot].status = 0;
	jobs[slot].hold = 0;
}

static void job_sleep(int slot)
{
	jobs[slot].status = -1;               /* indefinite: pick_next skips it until a wake */
	jobs[slot].hold = 0;
}

static int waiter_live(int i)
{
	return jobs[i].in_use && jobs[i].id == jwait_id[i] && jobs[i].status < 0;
}

static int rsv_add(int slot, unsigned int e);
static void rsv_release(int slot);
static void wake_space(void);

/* ---- reservations (Phase 2B cache-lifetime fix) -------------------------------------------------------------------
 * A fill that completes is reserved for every reader that was asleep on it: the sector stays VALID and is not evicted
 * until that reader has used it (its next probe commits, or it ends).  A reader's reservations accumulate across its
 * restarts, so a whole-operation restart makes progress.  Each reader holds at most R20_RSV_MAX.  Reservation is
 * generation-safe: a list entry whose entry has been freed or re-filled is stale and is skipped. */
static int rsv_compact(int slot)
{
	unsigned int k, n = 0;

	for (k = 0; k < rsv_n[slot]; k++) {
		unsigned int e = rsv_e[slot][k];

		if (e < nent && (ce[e].gen & GEN_MASK) == rsv_g[slot][k] && (ce[e].rmask & (1u << slot))) {
			rsv_e[slot][n] = rsv_e[slot][k];
			rsv_g[slot][n] = rsv_g[slot][k];
			n++;
		}
	}
	rsv_n[slot] = (unsigned char) n;
	return (int) n;
}

static int rsv_add(int slot, unsigned int e)
{
	unsigned int k;

	if (!c_on || slot < 0 || slot >= JOB_MAX || e >= nent)
		return 0;
	rsv_compact(slot);
	for (k = 0; k < rsv_n[slot]; k++)
		if (rsv_e[slot][k] == e)
			return 1;
	if (rsv_n[slot] >= R20_RSV_MAX) {
		st[R20_STAT_RSV_FULL]++;
		return 0;
	}
	k = rsv_n[slot]++;
	rsv_e[slot][k] = (unsigned short) e;
	rsv_g[slot][k] = ce[e].gen & GEN_MASK;
	ce[e].rmask |= 1u << slot;
	st[R20_STAT_RSV_SET]++;
	return 1;
}

static void rsv_release(int slot)
{
	unsigned int k, bit;
	int any = 0;

	if (!c_on || slot < 0 || slot >= JOB_MAX)
		return;
	bit = 1u << slot;
	for (k = 0; k < rsv_n[slot]; k++) {
		unsigned int e = rsv_e[slot][k];

		if (e < nent && (ce[e].gen & GEN_MASK) == rsv_g[slot][k] && (ce[e].rmask & bit)) {
			ce[e].rmask &= ~bit;
			st[R20_STAT_RSV_RELEASE]++;
			any = 1;
		}
	}
	rsv_n[slot] = 0;
	if (any)
		wake_space();                 /* an entry may have become evictable */
}

/* True if some entry could be taken now (free, or idle VALID/ERROR), without moving the clock hand. */
static int victim_free(void)
{
	unsigned int e;

	for (e = 0; e < nent; e++)
		if (ce[e].state == CE_FREE)
			return 1;
	for (e = 0; e < nent; e++)
		if (ce[e].state != CE_LOADING && !ce[e].pins && !ce[e].rmask)
			return 1;
	return 0;
}

/* Wake the jobs asleep for entry e (fill completed, or invalidated).  Only sleepers that still wait for it. */
static void wake_entry_ex(unsigned int e, int reserve)
{
	int i;

	for (i = 0; i < JOB_MAX; i++) {
		if (jwait[i] != (unsigned short) (e + 1u))
			continue;
		jwait[i] = 0;
		if (waiter_live(i)) {
			if (reserve)
				rsv_add(i, e);      /* the completed sector is kept for this reader until it has used it */
			job_wake(i);
			st[R20_STAT_WAKE_CACHE]++;
		}
	}
}

static void wake_entry(unsigned int e)
{
	wake_entry_ex(e, 0);
}

/* Wake the jobs waiting for a free cache entry: any entry may have become free or ready. */
static void wake_space(void)
{
	int i;

	for (i = 0; i < JOB_MAX; i++) {
		if (jwait[i] != (unsigned short) NOSPACE_TAG)
			continue;
		jwait[i] = 0;
		if (waiter_live(i)) {
			job_wake(i);
			st[R20_STAT_WAKE_CACHE]++;
		}
	}
}

/* The worker's body for a user request: synthetic CPU work, run with the BKL released. */
static int r20w_body(unsigned int op, unsigned int arg)
{
	volatile unsigned int acc = 0;
	unsigned int i;

	if (op != R20_OP_SPIN || arg > R20_SPIN_MAX)
		return R20_ERR_ARG;
	for (i = 0; i < arg; i++)
		acc += (i ^ 0x5au);
	return (int) (((arg * 3u) + 1u) & 0x7fffffffu);
}

/* The worker job.  Ring 0, its own supervisor stack.  Every line that changes shared state is a trap.  A cache fill
 * runs with interrupts off (the frame of its TAKE has IF clear): no timer interrupt can enter the kernel and need the
 * BKL while this CPU holds the storage controller. */
static void r20w_entry(void)
{
	for (;;) {
		int h = qtrap(QT_R20_TAKE, 0, 0, 0);

		if (h == 0)
			continue;                       /* woken with nothing pending: take again */
		if (h < 0) {
			wstate = R20W_STOPPED;          /* fail-stop: the worker owns no request any more */
			return;
		}
		if ((unsigned int) h & R20_CACHE_BIT) {
			unsigned int e = (unsigned int) h & (R20C_MAX_ENT - 1u);
			int res;

#ifdef R20C_INJECT_EVERY
			/* test-only: every Nth fill fails at the device, alternately as an IDE error (-1) and a timeout (-2).
			 * The sector is not read; the entry goes to ERROR exactly as a real device failure would. */
			if (++inj_n % R20C_INJECT_EVERY == 0)
				res = (inj_n / R20C_INJECT_EVERY) & 1u ? -1 : -2;
			else
				res = disk_read_worker(ce[e].lba, cdata + e * R20C_SECTOR);   /* no BKL; the controller is ours */
#else
			res = disk_read_worker(ce[e].lba, cdata + e * R20C_SECTOR);       /* no BKL; the controller is ours */
#endif
			r20_claim = 0;
			qtrap(QT_R20_DONE, (unsigned int) h, (unsigned int) res, 0);
			continue;
		}
		{
			unsigned int info = (unsigned int) qtrap(QT_R20_INFO, (unsigned int) h, 0, 0);
			unsigned int op = info & 0xffu;
			unsigned int arg = info >> 8;
			int res = r20w_body(op, arg);       /* no BKL here */

			qtrap(QT_R20_DONE, (unsigned int) h, (unsigned int) res, 0);
		}
	}
}

int r20w_start(void)
{
	int s, n = smp_cpu_count();

	if (wslot >= 0)
		return 0;
	s = job_alloc("IOW0", r20w_entry, 1);
	if (s < 0)
		return -1;
	s &= 0xffff;
	/* Prefer a background CPU: the highest one when there is one, else CPU 0 (time-shared with applications). */
	jobs[s].aff = (signed char) (n >= 2 ? 1 : 0);
#ifdef QDOS_R16B
	jobs[s].pin = jobs[s].aff;             /* a manual pin: the policy never migrates the worker */
#endif
	wslot = s;
	wstate = R20W_READY;
	return 0;
}

int r20w_is_worker(int slot)
{
	return slot == wslot;
}

/* ---- Phase 1: the user request table ---------------------------------------------------------------------------- */

int r20w_trap(unsigned int fn, unsigned int a, unsigned int b, int caller_job, int *sleep)
{
	int r = R20_ERR_ARG;
	int i, best;
	struct job *cj = &jobs[caller_job];
	int ce_head;

	*sleep = 0;
	switch (fn) {
	case QT_R20_SUBMIT:
		if (wslot < 0 || wstate == R20W_STOPPED) {
			r = R20_ERR_NOWORK;
			break;
		}
		if (a != R20_OP_SPIN || b > R20_SPIN_MAX) {
			r = R20_ERR_ARG;
			break;
		}
		for (i = 0; i < R20_NQ; i++)
			if (rq[i].state == RQ_FREE)
				break;
		if (i == R20_NQ) {
			st[R20_STAT_FULL]++;
			r = R20_ERR_FULL;
			break;
		}
		rq[i].state = RQ_QUEUED;
		rq[i].op = (unsigned char) a;
		rq[i].waiting = 0;
		rq[i].gen = next_gen();
		rq[i].seq = seq_next++;
		rq[i].arg = b;
		rq[i].submitter_id = cj->id;
		rq[i].waiter_id = 0;
		rq[i].waiter_slot = -1;
		rq[i].result = 0;
		st[R20_STAT_SUBMITS]++;
		if (wstate == R20W_BLOCKED) {
			job_wake(wslot);
			wstate = R20W_READY;
			st[R20_STAT_WAKE_WORKER]++;
		}
		r = (int) handle_of(i);
		break;

	case QT_R20_WAIT:
		i = (int) (a & 0xffu);
		if (i >= R20_NQ || rq[i].state == RQ_FREE || rq[i].gen != ((a >> 8) & GEN_MASK)) {
			st[R20_STAT_STALE]++;
			r = R20_ERR_STALE;
			break;
		}
		if (rq[i].submitter_id != cj->id) {
			r = R20_ERR_OWNER;
			break;
		}
		if (rq[i].state == RQ_DONE) {
			r = rq[i].result;
			rq[i].state = RQ_FREE;
			rq[i].waiting = 0;
			st[R20_STAT_WAIT_DONE]++;
			break;
		}
		rq[i].waiting = 1;
		rq[i].waiter_id = cj->id;
		rq[i].waiter_slot = caller_job;
		job_sleep(caller_job);
		st[R20_STAT_WAIT_SLEPT]++;
		r = R20_RETRY;
		*sleep = 1;
		break;

	case QT_R20_TAKE:
		if (caller_job != wslot) {
			r = R20_ERR_OWNER;
			break;
		}
		best = -1;
		for (i = 0; i < R20_NQ; i++)
			if (rq[i].state == RQ_QUEUED && (best < 0 || rq[i].seq < rq[best].seq))
				best = i;
		ce_head = c_on ? r20c_queue_head() : -1;
		st[R20_STAT_TAKES]++;
		if (ce_head >= 0 && (best < 0 || ce[ce_head].seq < rq[best].seq)) {
			/* the oldest pending work is a cache fill: pop it, pin the entry, claim the controller (under the BKL) */
			unsigned int e = (unsigned int) ce_head;
			r20c_queue_pop();
			ce[e].queued = 0;
			ce[e].pins = 1;
			r20_claim = 1;
			wstate = R20W_RUNNING;
			r = (int) (R20_CACHE_BIT | ((ce[e].gen & GEN_MASK) << R20C_IDX_BITS) | e);
			break;
		}
		if (best < 0) {
			job_sleep(caller_job);
			wstate = R20W_BLOCKED;
			st[R20_STAT_WORKER_SLEPT]++;
			r = 0;
			*sleep = 1;
			break;
		}
		rq[best].state = RQ_ACTIVE;
		wstate = R20W_RUNNING;
		r = (int) handle_of(best);
		break;

	case QT_R20_INFO:
		i = (int) (a & 0xffu);
		if (caller_job != wslot) {
			r = R20_ERR_OWNER;
			break;
		}
		if (i >= R20_NQ || rq[i].state != RQ_ACTIVE || rq[i].gen != ((a >> 8) & GEN_MASK)) {
			st[R20_STAT_STALE]++;
			r = R20_ERR_STALE;
			break;
		}
		r = (int) ((rq[i].arg << 8) | rq[i].op);
		break;

	case QT_R20_DONE:
		if (caller_job != wslot) {
			r = R20_ERR_OWNER;
			break;
		}
		if (a & R20_CACHE_BIT) {
			r = r20c_fill_done(a, (int) b);
			break;
		}
		i = (int) (a & 0xffu);
		if (i >= R20_NQ || rq[i].gen != ((a >> 8) & GEN_MASK) ||
		    (rq[i].state != RQ_ACTIVE && rq[i].state != RQ_ABANDONED)) {
			st[R20_STAT_STALE]++;
			r = R20_ERR_STALE;
			break;
		}
		st[R20_STAT_COMPLETIONS]++;
		if (rq[i].state == RQ_ABANDONED) {
			rq[i].state = RQ_FREE;
			rq[i].waiting = 0;
			r = 0;
			break;
		}
		rq[i].result = (int) b;
		rq[i].state = RQ_DONE;
		if (rq[i].waiting) {
			int ws = rq[i].waiter_slot;
			if (ws >= 0 && jobs[ws].in_use && jobs[ws].id == rq[i].waiter_id && jobs[ws].status < 0) {
				job_wake(ws);
				st[R20_STAT_WAKE_SUBMITTER]++;
			}
			rq[i].waiting = 0;
		}
		r = 0;
		break;

	case QT_R20_STAT:
		if (a == R20_STAT_STATE)
			r = wstate;
		else if (a == R20_STAT_CACHE_USED)
			r = (int) r20c_used();
		else if (a == R20_STAT_MY_GIVEUP)
			r = (job_current >= 0 && job_current < JOB_MAX) ? (int) giveup_job[job_current] : 0;
		else if (a == R20_STAT_DUMP)
			r = r20c_dump();
		else if (a < R20_STAT_N)
			r = (int) st[a];
		else
			r = R20_ERR_ARG;
		break;

	default:
		r = R20_ERR_ARG;
		break;
	}
	return r;
}

void r20w_job_gone(int slot, unsigned int id)
{
	int i;

	(void) id;
	if (slot >= 0 && slot < JOB_MAX) {
		jwait[slot] = 0;
		misses[slot] = 0;
		rsv_release(slot);
	}
	for (i = 0; i < R20_NQ; i++) {
		if (rq[i].state == RQ_FREE || rq[i].submitter_id != id)
			continue;
		if (rq[i].state == RQ_QUEUED || rq[i].state == RQ_DONE) {
			rq[i].state = RQ_FREE;
			st[R20_STAT_GONE_FREED]++;
		} else if (rq[i].state == RQ_ACTIVE) {
			rq[i].state = RQ_ABANDONED;     /* the worker owns it until DONE; DONE frees it */
			st[R20_STAT_ABANDONED]++;
		}
		rq[i].waiting = 0;
	}
}

unsigned long r20w_stat(int idx)
{
	if (idx == R20_STAT_STATE)
		return (unsigned long) wstate;
	if (idx == R20_STAT_CACHE_USED)
		return (unsigned long) r20c_used();
	return (idx >= 0 && idx < R20_STAT_N) ? st[idx] : 0ul;
}

/* ---- Phase 2B: the bounded sector cache ------------------------------------------------------------------------- */

static unsigned int hsh(unsigned int lba)
{
	unsigned int x = lba * 0x9e3779b1u;
	return (x ^ (x >> 15)) & (nb - 1u);
}

/* Index of the entry holding lba (hashed), or -1. */
static int cache_find(unsigned int lba)
{
	unsigned short i;

	for (i = bucket[hsh(lba)]; i != 0; i = ce[i - 1u].hnext)
		if (ce[i - 1u].hashed && ce[i - 1u].lba == lba)
			return (int) (i - 1u);
	return -1;
}

static void cache_unhash(unsigned int e)
{
	unsigned short *p = &bucket[hsh(ce[e].lba)];

	while (*p != 0) {
		if (*p == (unsigned short) (e + 1u)) {
			*p = ce[e].hnext;
			break;
		}
		p = &ce[*p - 1u].hnext;
	}
	ce[e].hashed = 0;
	ce[e].hnext = 0;
}

static void cache_hash(unsigned int e)
{
	unsigned int h = hsh(ce[e].lba);

	ce[e].hnext = bucket[h];
	bucket[h] = (unsigned short) (e + 1u);
	ce[e].hashed = 1;
}

/* Clock (second chance) victim: a FREE entry, or a VALID/ERROR one that is not pinned or recently used.  Bounded:
 * at most two sweeps, so a full cache with every entry loading or pinned returns -1 (the caller waits). */
static int cache_victim(void)
{
	unsigned int n;

	for (n = 0; n < 2u * nent; n++) {
		unsigned int e = hand;

		hand = (hand + 1u == nent) ? 0u : hand + 1u;
		if (ce[e].state == CE_FREE)
			return (int) e;
		if (ce[e].state == CE_LOADING || ce[e].pins || ce[e].rmask)
			continue;
		if (ce[e].ref) {
			ce[e].ref = 0;
			continue;
		}
		return (int) e;
	}
	return -1;
}

/* Take entry e for a new value: drop what it held (an eviction counts), and make it hashed for lba. */
static void cache_take(unsigned int e, unsigned int lba)
{
	if (ce[e].state == CE_VALID)
		st[R20_STAT_EVICT]++;
	if (ce[e].hashed)
		cache_unhash(e);
	ce[e].lba = lba;
	ce[e].gen = next_gen();
	ce[e].ref = 0;
	ce[e].dev = 0;
	ce[e].pins = 0;
	ce[e].queued = 0;
	ce[e].rmask = 0;
	ce[e].state = CE_LOADING;
	cache_hash(e);
}

/* The queue ring: a ticket is pushed when an entry starts loading and popped when the worker takes it. */
static void cq_push(unsigned int e)
{
	cq[(cq_head + cq_count) % nent] = (ce[e].gen << R20C_IDX_BITS) | e;
	cq_count++;
}

/* The oldest queued fill, or -1.  A ticket of an orphan (invalidated while queued) is dropped here; its entry is
 * freed when it is the orphan's last reference. */
int r20c_queue_head(void)
{
	while (cq_count != 0) {
		unsigned int t = cq[cq_head];
		unsigned int e = t & (R20C_MAX_ENT - 1u);
		unsigned int g = t >> R20C_IDX_BITS;

		if (ce[e].queued && (ce[e].gen & GEN_MASK) == g)
			return (int) e;
		if (ce[e].queued) {                       /* orphan: invalidated while it waited */
			ce[e].queued = 0;
			if (!ce[e].hashed && ce[e].pins == 0)
				ce[e].state = CE_FREE;
			ce[e].rmask = 0;
			wake_space();
		}
		cq_head = (cq_head + 1u) % nent;
		cq_count--;
	}
	return -1;
}

void r20c_queue_pop(void)
{
	cq_head = (cq_head + 1u) % nent;
	cq_count--;
}

/* A fill's DONE.  A matching generation publishes the result; an orphan publishes nothing. */
int r20c_fill_done(unsigned int ticket, int res)
{
	unsigned int e = ticket & (R20C_MAX_ENT - 1u);
	unsigned int g = (ticket >> R20C_IDX_BITS) & GEN_MASK;

	if (e >= nent) {
		st[R20_STAT_STALE]++;
		return R20_ERR_STALE;
	}
	if ((ce[e].gen & GEN_MASK) != g) {
		/* orphan: invalidated while in flight.  The data is discarded. */
		ce[e].pins = 0;
		if (!ce[e].hashed && !ce[e].queued && ce[e].state == CE_LOADING)
			ce[e].state = CE_FREE;
		st[R20_STAT_FILL_DISCARDED]++;
		wake_space();
		return 0;
	}
	ce[e].pins = 0;
	st[R20_STAT_COMPLETIONS]++;
	if (res == 0) {
		ce[e].state = CE_VALID;
		ce[e].ref = 1;
		st[R20_STAT_FILL_OK]++;
		wake_entry_ex(e, 1);        /* reserve it for every reader that waits on it */
	} else {
		ce[e].state = CE_ERROR;
		st[R20_STAT_FILL_ERR]++;
		wake_entry_ex(e, 1);        /* the failure is kept for its waiters: each reader that asked is told once */
	}
	wake_space();
	return 0;
}

/* Stall diagnostics (R20_STAT_DUMP): the entry states, the fill queue, the storage claim, and every job asleep on the
 * cache with what it waits for.  One console line per item; returns the entry count. */
static int r20c_dump(void)
{
	unsigned int e, i;
	unsigned long cnt[4] = { 0, 0, 0, 0 };
	unsigned long waiters = 0, nospace = 0;

	if (!c_on)
		return 0;
	for (e = 0; e < nent; e++)
		cnt[ce[e].state & 3u]++;
	for (i = 0; i < JOB_MAX; i++)
		if (jwait[i] != 0) {
			waiters++;
			if (jwait[i] == NOSPACE_TAG)
				nospace++;
		}
	con_puts("R20DUMP entries="); con_putdec(nent);
	con_puts(" free="); con_putdec(cnt[CE_FREE]);
	con_puts(" loading="); con_putdec(cnt[CE_LOADING]);
	con_puts(" valid="); con_putdec(cnt[CE_VALID]);
	con_puts(" error="); con_putdec(cnt[CE_ERROR]);
	con_puts(" queue="); con_putdec(cq_count);
	con_puts(" worker="); con_putdec((unsigned long) wstate);
	con_puts(" claim="); con_putdec((unsigned long) r20_claim);
	con_puts(" waiters="); con_putdec(waiters);
	con_puts(" nospace="); con_putdec(nospace);
	con_puts(" gaveup="); con_putdec(st[R20_STAT_GAVE_UP]);
	con_puts("\n");
	con_puts("R20DUMP counters hit="); con_putdec(st[R20_STAT_HIT]);
	con_puts(" miss="); con_putdec(st[R20_STAT_MISS]);
	con_puts(" fill="); con_putdec(st[R20_STAT_FILL]);
	con_puts(" fill_ok="); con_putdec(st[R20_STAT_FILL_OK]);
	con_puts(" fill_err="); con_putdec(st[R20_STAT_FILL_ERR]);
	con_puts(" coal="); con_putdec(st[R20_STAT_COALESCED]);
	con_puts(" evict="); con_putdec(st[R20_STAT_EVICT]);
	con_puts(" nospace="); con_putdec(st[R20_STAT_NOSPACE]);
	con_puts(" restart="); con_putdec(st[R20_STAT_RESTART]);
	con_puts(" invalidate="); con_putdec(st[R20_STAT_INVALIDATE]);
	con_puts(" discarded="); con_putdec(st[R20_STAT_FILL_DISCARDED]);
	con_puts(" ide_read="); con_putdec(st[R20_STAT_IDE_READ]);
	con_puts("\n");
	{
		unsigned long bits = 0, held = 0;
		int j;

		for (e = 0; e < nent; e++)
			for (j = 0; j < JOB_MAX; j++)
				if (ce[e].rmask & (1u << j))
					bits++;
		for (j = 0; j < JOB_MAX; j++)
			held += rsv_n[j];
		con_puts("R20DUMP reservations bits="); con_putdec(bits);
		con_puts(" held="); con_putdec(held);
		con_puts(" set="); con_putdec(st[R20_STAT_RSV_SET]);
		con_puts(" release="); con_putdec(st[R20_STAT_RSV_RELEASE]);
		con_puts(" full="); con_putdec(st[R20_STAT_RSV_FULL]);
		con_puts(" sync_fallback="); con_putdec(st[R20_STAT_SYNC_FALLBACK]);
		con_puts("\n");
	}
	for (i = 0; i < JOB_MAX; i++)
		if (jwait[i] != 0) {
			con_puts("R20DUMP job="); con_putdec(i);
			con_puts(" waits="); con_putdec(jwait[i] == NOSPACE_TAG ? 0ul : (unsigned long) (jwait[i] - 1u));
			con_puts(jwait[i] == NOSPACE_TAG ? " (cache full)" : " (fill)");
			con_puts(" misses="); con_putdec(misses[i]);
			con_puts(" giveups="); con_putdec(giveup_job[i]);
			con_puts("\n");
		}
	return (int) nent;
}

/* Count of VALID entries (diagnostics). */
unsigned long r20c_used(void)
{
	unsigned int e;
	unsigned long n = 0;

	if (!c_on)
		return 0;
	for (e = 0; e < nent; e++)
		if (ce[e].state == CE_VALID)
			n++;
	return n;
}

/* Remove lba from the cache: a write, before or after the disk write.  A VALID or ERROR entry is freed; a LOADING one
 * becomes an orphan (its waiters wake and find nothing). */
static void cache_invalidate(unsigned int lba)
{
	int e = cache_find(lba);

	if (e < 0)
		return;
	cache_unhash((unsigned int) e);
	ce[e].gen = next_gen();
	if (ce[e].state == CE_VALID || ce[e].state == CE_ERROR) {
		ce[e].state = CE_FREE;
		ce[e].rmask = 0;             /* a write frees the sector: any reservation of it is void */
		ce[e].pins = 0;
	} else {
		wake_entry((unsigned int) e);
	}
	st[R20_STAT_INVALIDATE]++;
	wake_space();
}

/* Boot: size the cache from firmware-reported usable RAM, capped at R20C_BUDGET_MAX, and allocate it from the heap. */
void r20c_init(void)
{
	struct mm_summary s;
	unsigned long budget, per;
	unsigned int n, i;
	unsigned long bytes;
	void *p;

	c_on = 0;
	memmap_summary(&s);
	budget = (unsigned long) s.usable_kib * 1024ul * R20C_PCT / 100ul;
	if (budget > R20C_BUDGET_MAX)
		budget = R20C_BUDGET_MAX;
	/* per entry: sector data, metadata, one queue slot, and up to one bucket */
	per = R20C_SECTOR + sizeof(struct r20_ce) + sizeof(unsigned int) + sizeof(unsigned short);
	n = (unsigned int) (budget / per);
	if (n > R20C_MAX_ENT)
		n = R20C_MAX_ENT;
	if (n < R20C_MIN_ENT)
		n = R20C_MIN_ENT;
#ifdef R20C_TEST_ENT
	if (n > R20C_TEST_ENT)                   /* TEST ONLY (a separate build): a tiny cache exposes eviction and no-space */
		n = R20C_TEST_ENT;
#endif
	for (; n >= R20C_MIN_ENT; n /= 2) {
		unsigned int m = 1u;

		while ((m << 1) <= n)                /* buckets: the largest power of two not above n */
			m <<= 1;
		p = mem_alloc(n * sizeof(struct r20_ce), R20C_OWNER);
		if (mem_is_error((unsigned int) p))
			continue;
		ce = (struct r20_ce *) p;
		p = mem_alloc(n * R20C_SECTOR, R20C_OWNER);
		if (mem_is_error((unsigned int) p)) {
			mem_free(ce, R20C_OWNER);
			ce = 0;
			continue;
		}
		cdata = (unsigned char *) p;
		p = mem_alloc(m * sizeof(unsigned short), R20C_OWNER);
		if (mem_is_error((unsigned int) p)) {
			mem_free(ce, R20C_OWNER);
			mem_free(cdata, R20C_OWNER);
			ce = 0;
			cdata = 0;
			continue;
		}
		bucket = (unsigned short *) p;
		p = mem_alloc(n * sizeof(unsigned int), R20C_OWNER);
		if (mem_is_error((unsigned int) p)) {
			mem_free(ce, R20C_OWNER);
			mem_free(cdata, R20C_OWNER);
			mem_free(bucket, R20C_OWNER);
			ce = 0;
			cdata = 0;
			bucket = 0;
			continue;
		}
		cq = (unsigned int *) p;
		nent = n;
		nb = m;
		break;
	}
	if (ce == 0)
		return;                               /* no memory for even the minimum: file reads run synchronously */
	for (i = 0; i < nent; i++) {
		ce[i].state = CE_FREE;
		ce[i].hashed = 0;
		ce[i].queued = 0;
		ce[i].pins = 0;
		ce[i].ref = 0;
		ce[i].hnext = 0;
	}
	hand = 0;
	cq_head = 0;
	cq_count = 0;
	c_on = 1;
	bytes = (unsigned long) nent * (R20C_SECTOR + sizeof(struct r20_ce) + sizeof(unsigned int)) +
		(unsigned long) nb * sizeof(unsigned short);
	st[R20_STAT_CACHE_ENTRIES] = nent;
	st[R20_STAT_CACHE_BYTES] = bytes;
}

/* Wait (bounded) until the controller is free.  A sync trap holds the BKL; the worker clears the claim without it. */
int r20c_wait_dev(void)
{
	unsigned long n;

	if (!r20_claim)
		return 0;
	st[R20_STAT_DEV_WAIT]++;
	for (n = 0; r20_claim && n < R20_DEV_SPIN; n++)
		;
	return r20_claim ? -1 : 0;
}

/* Any sector read.  A probe answers from the cache or asks for a restart.  A non-probe read (another trap's file
 * operation, a mount or directory read) may use the controller itself once the claim is free, and keeps the sector. */
int r20c_read(unsigned int lba, unsigned char *buf)
{
	int e;
	unsigned int i;

	if (!c_on) {
		if (r20c_wait_dev() != 0)
			return FAT_ERR_IO;
		st[R20_STAT_IDE_READ]++;
		return disk_read_body(lba, buf);
	}
	e = cache_find(lba);
	if (e >= 0 && ce[e].state == CE_VALID) {
		for (i = 0; i < R20C_SECTOR; i++)
			buf[i] = cdata[(unsigned int) e * R20C_SECTOR + i];
		ce[e].ref = 1;
		st[R20_STAT_HIT]++;
		return 0;
	}
	if (e >= 0 && ce[e].state == CE_ERROR) {
		/* reported once; the next read tries the disk again */
		cache_unhash((unsigned int) e);
		ce[e].state = CE_FREE;
		ce[e].rmask = 0;
		wake_space();
		return FAT_ERR_IO;
	}
	if (!r20_probe) {
		int rc, v;

		if (r20c_wait_dev() != 0)
			return FAT_ERR_IO;
		st[R20_STAT_IDE_READ]++;
		st[R20_STAT_SYNC_READ]++;
		rc = disk_read_body(lba, buf);
		if (rc == 0 && e < 0) {            /* keep it: a sync read warms the cache too (no queue, no pin) */
			v = cache_victim();
			if (v >= 0) {
				cache_take((unsigned int) v, lba);
				ce[v].state = CE_VALID;
				ce[v].ref = 1;
				for (i = 0; i < R20C_SECTOR; i++)
					cdata[(unsigned int) v * R20C_SECTOR + i] = buf[i];
			}
		}
		return rc;
	}
	st[R20_STAT_MISS]++;
	if (e < 0 && job_current >= 0 && job_current < JOB_MAX && rsv_compact(job_current) > 0 &&
	    (rsv_n[job_current] >= R20_RSV_MAX || !victim_free())) {
		/* This reader holds sectors it has not used yet and cannot take another entry.  Waiting for an entry could
		 * deadlock with other readers in the same position, so this one sector is read directly, uncached.  The read
		 * completes; the uncached sector is not kept. */
		st[R20_STAT_SYNC_FALLBACK]++;
		if (r20c_wait_dev() != 0)
			return FAT_ERR_IO;
		st[R20_STAT_IDE_READ]++;
		return disk_read_body(lba, buf);
	}
	r20_miss_lba = lba;
	return FAT_ERR_MISS;
}

/* A write invalidates the sector before and after the disk write, whatever the outcome, and never races the worker. */
int r20c_write(unsigned int lba, const unsigned char *buf)
{
	int rc;

	if (r20c_wait_dev() != 0)
		return -1;
	if (c_on)
		cache_invalidate(lba);
	st[R20_STAT_IDE_WRITE]++;
	rc = disk_write_body(lba, buf);
	if (c_on)
		cache_invalidate(lba);
	return rc;
}

/* Mount or media change: nothing cached is trusted.  Orphans finish into dead generations. */
void r20c_invalidate_all(void)
{
	unsigned int e, i;

	if (!c_on)
		return;
	(void) r20c_wait_dev();
	for (e = 0; e < nent; e++) {
		ce[e].gen = next_gen();
		ce[e].hashed = 0;
		ce[e].hnext = 0;
		if (ce[e].state == CE_VALID || ce[e].state == CE_ERROR) {
			ce[e].state = CE_FREE;
			ce[e].rmask = 0;
			ce[e].pins = 0;
		} else if (ce[e].state == CE_LOADING) {
			wake_entry(e);
		}
	}
	for (i = 0; i < nb; i++)
		bucket[i] = 0;
	st[R20_STAT_INVAL_ALL]++;
	wake_space();
}

/* A probed file read missed: queue the fill (or join the one in flight) and sleep.  The caller re-executes the read
 * from the start when it wakes.  Bounded per process (R20_MAX_MISS). */
static int r20c_block(unsigned int lba)
{
	int cur = job_current;
	int e;

	if (cur < 0 || cur >= JOB_MAX || wslot < 0 || wstate == R20W_STOPPED)
		return FAT_ERR_IO;                    /* no worker: fail the read rather than sleep forever */
	if (misses[cur] >= R20_MAX_MISS) {
		misses[cur] = 0;
		st[R20_STAT_GAVE_UP]++;
		giveup_job[cur]++;
		return FAT_ERR_IO;                    /* bounded: a process cannot restart forever */
	}
	e = cache_find(lba);
	if (e >= 0 && ce[e].state == CE_LOADING) {
		misses[cur]++;
		st[R20_STAT_COALESCED]++;           /* one fill serves every waiter for this sector */
	} else {
		int v = cache_victim();
		if (v < 0) {
			/* every entry is loading or pinned: wait for any of them to finish (no spinning, no allocation).
			 * Backpressure is not a failure: it does not use the per-process miss budget, because every wake-up
			 * means an entry has just finished. */
			jwait[cur] = (unsigned short) NOSPACE_TAG;
			jwait_id[cur] = jobs[cur].id;
			job_sleep(cur);
			st[R20_STAT_NOSPACE]++;
			st[R20_STAT_RESTART]++;
			return R20_RESTART;
		}
		misses[cur]++;
		cache_take((unsigned int) v, lba);
		e = v;
		ce[e].queued = 1;
		ce[e].seq = seq_next++;
		cq_push((unsigned int) e);
		st[R20_STAT_FILL]++;
		if (wstate == R20W_BLOCKED) {
			job_wake(wslot);
			wstate = R20W_READY;
		}
	}
	jwait[cur] = (unsigned short) (e + 1);
	jwait_id[cur] = jobs[cur].id;
	job_sleep(cur);
	st[R20_STAT_RESTART]++;
	return R20_RESTART;
}

/* A file read in two steps: a probe on a copy (nothing changes), then either the committed result or a restart.  The
 * probe's data goes to a kernel buffer; the user buffer is written only on commit. */
int r20_file_read(struct fat16_file *ff, unsigned char *ubuf, unsigned int n)
{
	struct fat16_file tf;
	unsigned char kb[CHAN_MAX_WRITE];
	unsigned int got = 0, i;
	int rc, cur = job_current;

	if (!c_on) {                              /* no cache: the plain synchronous read (Phase 2A without the cache) */
		rc = fat16_read(ff, ubuf, n, &got);
		return rc != FAT_OK ? rc : (int) got;
	}
	if (n > CHAN_MAX_WRITE)
		return FAT_ERR_RANGE;
	tf = *ff;
	r20_probe = 1;
	rc = fat16_read(&tf, kb, n, &got);
	r20_probe = 0;
	if (rc == FAT_ERR_MISS) {
		int b = r20c_block(r20_miss_lba);

		if (b != R20_RESTART)                 /* a restart keeps the reservations: the next probe uses them */
			rsv_release(cur);
		return b;
	}
	rsv_release(cur);                         /* the read ends (committed or failed): its sectors may now be evicted */
	if (rc != FAT_OK)
		return rc;
	if (cur >= 0 && cur < JOB_MAX)
		misses[cur] = 0;
	*ff = tf;
	for (i = 0; i < got; i++)
		ubuf[i] = kb[i];
	return (int) got;
}

#endif
