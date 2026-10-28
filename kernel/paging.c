/*
 * QDOS/386 R11J/R11K - paging.  See paging.h, docs/R11J-ADDRESS-SPACE-ABI.md and docs/R11K-PAGING-APP-ABI.md.
 * Compiled only into -DQDOS_PAGING images (Makefile: I386EXTRA / PCEXTRA).
 *
 * Page-table memory is a static, page-aligned pool in kernel BSS (below HEAP_END), so it is
 * identity-mapped in every directory and the CPU's table walk always finds it in RAM.
 */
#include "paging.h"
#include "jobs.h"
#include "memory.h"
#include "platform.h"
#include "aout.h"
#include "channel.h"
#include "irq.h"

extern unsigned int irq_save(void);
extern void irq_restore(unsigned int flags);

/* aout.c (QDOS_PAGING): frame runs from the application region pool. */
long app_frames_alloc(unsigned int lo, unsigned int hi, unsigned int bytes,
                      unsigned int owner, unsigned int *base);
int app_frames_release(long handle, unsigned int owner);
void app_pool_stats(unsigned int *count, unsigned int *bytes);
/* jobs.c (QDOS_PAGING): end the current job from an exception frame. */
unsigned int job_kill_current(unsigned int sp);

#define PG_TPL_PAGES   (1 + PG_DIRS_USED)                 /* directory + identity tables */
#define PG_REC_PAGES   (1 + PG_PROC_TABLES)
#define PG_POOL_PAGES  (PG_TPL_PAGES + PG_PROC_MAX * PG_REC_PAGES)

static unsigned char pg_mem[(PG_POOL_PAGES + 1) * PG_PAGE];   /* +1: alignment slack */

#define REC_FREE   0
#define REC_BUILT  1     /* directory built, not yet bound to a job */
#define REC_LIVE   2

struct pg_rec {
	int state;
	unsigned int job_id;
	int slot;
	long frames_h;
	unsigned int frames;
	unsigned int dir;                       /* physical == virtual (identity) */
	unsigned int pages[PG_REC_PAGES];       /* [0] directory, then private tables */
};

static unsigned int pg_base;                /* first aligned page of pg_mem */
static unsigned int tpl_dir;
static struct pg_rec recs[PG_PROC_MAX];
static int slot_rec[JOB_MAX];               /* job slot -> record, or -1 */
static unsigned char hold_pending[JOB_MAX];
static unsigned int cur_cr3;
static unsigned int cr3_loads;
static unsigned int holds_deferred;
static unsigned int held_bytes;
static int pg_on;
static int exc_depth;

static unsigned int page_at(unsigned int i)
{
	return pg_base + i * PG_PAGE;
}

static void put_hex(unsigned int v)
{
	static const char hx[] = "0123456789abcdef";
	int s;

	for (s = 28; s >= 0; s -= 4)
		con_putc((unsigned char) hx[(v >> s) & 15u]);
}

static void copy_page(unsigned int dst, unsigned int src)
{
	unsigned int *d = (unsigned int *) dst;
	unsigned int *s = (unsigned int *) src;
	unsigned int i;

	for (i = 0; i < 1024u; i++)
		d[i] = s[i];
}

static int in_window(unsigned int va)
{
	return va >= PG_VBASE && va < PG_VEND;
}

static int in_pool(unsigned int va)
{
	return va >= PG_POOL_LO && va < PG_POOL_HI0;
}

/* Does the 4 MiB range of directory entry i hold any window or pool page? */
static int dir_touched(unsigned int i)
{
	unsigned int lo = i * PG_DIR_SPAN, hi = lo + PG_DIR_SPAN;

	return (PG_VBASE < hi && lo < PG_VEND) || (PG_POOL_LO < hi && lo < PG_POOL_HI0);
}

/* Is the whole 4 MiB range of entry i inside the pool (so it is simply not present)? */
static int dir_all_pool(unsigned int i)
{
	unsigned int lo = i * PG_DIR_SPAN;

	return lo >= PG_POOL_LO && lo + PG_DIR_SPAN <= PG_POOL_HI0;
}

void pg_init(void)
{
	unsigned int i, k, *dir, *pt;

	pg_base = ((unsigned int) pg_mem + PG_PAGE - 1u) & ~(PG_PAGE - 1u);
	tpl_dir = page_at(0);
	dir = (unsigned int *) tpl_dir;
	for (i = 0; i < 1024u; i++)
		dir[i] = 0;
	for (i = 0; i < PG_DIRS_USED; i++) {
		pt = (unsigned int *) page_at(1 + i);
		for (k = 0; k < 1024u; k++)
			pt[k] = ((i * 1024u + k) * PG_PAGE) | PG_P | PG_RW;   /* supervisor */
		dir[i] = (unsigned int) pt | PG_P | PG_RW | PG_US;
	}
	for (i = 0; i < (unsigned int) PG_PROC_MAX; i++)
		recs[i].state = REC_FREE;
	for (i = 0; i < (unsigned int) JOB_MAX; i++) {
		slot_rec[i] = -1;
		hold_pending[i] = 0;
	}
	pg_load_cr3(tpl_dir);
	cur_cr3 = tpl_dir;
	pg_enable();
	pg_on = 1;
	/* Kept short: the PC boot sector loads at most 0xF000 bytes of kernel + transfer area. */
	con_puts("PAGING: on, template cr3 ");
	put_hex(pg_read_cr3());
	con_puts(" pool ");
	put_hex(PG_POOL_LO);
	con_puts("..");
	put_hex(PG_POOL_HI);
	con_puts("\n");
}

static unsigned int dir_of_slot(int slot)
{
	if (slot >= 0 && slot < JOB_MAX && slot_rec[slot] >= 0)
		return recs[slot_rec[slot]].dir;
	return tpl_dir;
}

void pg_activate(int slot)
{
	unsigned int d = dir_of_slot(slot);

	if (d != cur_cr3) {
#ifdef QDOS_TEST_PG_INT_AROUND_CR3
		/* TEST ONLY: an interrupt through the IDT immediately before and immediately after
		 * the CR3 load (int 0x27, the existing guest-test injection vector). */
		sw_int27();
#endif
		pg_load_cr3(d);
#ifdef QDOS_TEST_PG_INT_AROUND_CR3
		sw_int27();
#endif
		cur_cr3 = d;
		cr3_loads++;
	}
}

/* Poll and schedule-list tasks run from the timer ISR and scheduler in whatever directory is
 * active, but their nodes and code were registered by unpaged jobs (paged callers are refused),
 * so they are run in the template.  Returns the directory to restore. */
unsigned int pg_enter_template(void)
{
	unsigned int was = cur_cr3;

	if (was != tpl_dir) {
		pg_load_cr3(tpl_dir);
		cur_cr3 = tpl_dir;
		cr3_loads++;
	}
	return was;
}

void pg_leave_template(unsigned int was)
{
	if (was != cur_cr3) {
		pg_load_cr3(was);
		cur_cr3 = was;
		cr3_loads++;
	}
}

int pg_slot_paged(int slot)
{
	return slot >= 0 && slot < JOB_MAX && slot_rec[slot] >= 0;
}

/* A hold flag in the window or the pool means different memory in different directories
 * (the window is private to each paged process; the pool is mapped only in the template).
 * Such a write is made only while the owner's directory is active, whether the owner is paged
 * or not; otherwise it is deferred to pg_after_switch.  Flags elsewhere (heap, kernel) are the
 * same in every directory and are written at once. */
void pg_hold_release(int slot)
{
	struct job *j = &jobs[slot];
	unsigned int a = (unsigned int) j->hold;

	if (a == 0)
		return;
	if ((in_window(a) || in_pool(a)) && dir_of_slot(slot) != cur_cr3) {
		hold_pending[slot] = 1;       /* written when the owner's directory is active */
		holds_deferred++;
		return;
	}
	*j->hold = 0xff;
}

void pg_after_switch(int slot)
{
	if (slot < 0 || slot >= JOB_MAX || !hold_pending[slot])
		return;
	hold_pending[slot] = 0;
	if (jobs[slot].hold != 0 && dir_of_slot(slot) == cur_cr3)
		*jobs[slot].hold = 0xff;
}

int pg_user_range_ok(unsigned int addr, unsigned int n)
{
	unsigned int end;

	if (!pg_slot_paged(job_current))
		return -1;
	if (addr == 0 && n != 0)
		return 0;
	end = addr + n;
	if (end < addr)
		return 0;
	if (addr >= PG_VBASE && end <= PG_VEND)
		return 1;                     /* the caller's own window */
	if (mem_arena_base() != 0 && addr >= mem_arena_base() && end <= HEAP_END)
		return 1;                     /* shared kernel heap: its stack and QT_MEM_ALLOC blocks */
	return 0;
}

int pg_rec_alloc(void)
{
	int r;

	for (r = 0; r < PG_PROC_MAX; r++)
		if (recs[r].state == REC_FREE) {
			recs[r].state = REC_BUILT;
			recs[r].job_id = 0;
			recs[r].slot = -1;
			recs[r].frames_h = 0;
			recs[r].frames = 0;
			return r;
		}
	return PG_ERR_NOREC;
}

void pg_rec_build(int rec, unsigned int frames)
{
	struct pg_rec *p = &recs[rec];
	unsigned int i, k, n = 1, va, *dir, *pt;

#ifdef QDOS_TEST_PG_SHARE_FRAMES
	{
		/* TEST ONLY, negative control: every process maps the frames of the first one built,
		 * so the copies share their window.  The proof programs must detect it and fail. */
		static unsigned int first_frames;
		if (first_frames == 0)
			first_frames = frames;
		frames = first_frames;
	}
#endif
	for (i = 0; i < (unsigned int) PG_REC_PAGES; i++)
		p->pages[i] = page_at(PG_TPL_PAGES + (unsigned int) rec * PG_REC_PAGES + i);
	p->dir = p->pages[0];
	copy_page(p->dir, tpl_dir);
	dir = (unsigned int *) p->dir;
	for (i = 0; i < PG_DIRS_USED; i++) {
		if (!dir_touched(i))
			continue;                    /* shared template table */
		if (dir_all_pool(i)) {
			dir[i] = 0;                  /* whole 4 MiB is pool: not present */
			continue;
		}
		pt = (unsigned int *) p->pages[n++];
		copy_page((unsigned int) pt, dir[i] & ~(PG_PAGE - 1u));
		for (k = 0; k < 1024u; k++) {
			va = i * PG_DIR_SPAN + k * PG_PAGE;
			if (in_window(va))
				pt[k] = (frames + (va - PG_VBASE)) | PG_P | PG_RW | PG_US;
			else if (in_pool(va))
				pt[k] = 0;
		}
		dir[i] = (unsigned int) pt | PG_P | PG_RW | PG_US;
	}
	p->frames = frames;
}

void pg_rec_bind(int rec, int slot, unsigned int job_id, long frames_handle,
                 unsigned int frames)
{
	struct pg_rec *p = &recs[rec];

	p->state = REC_LIVE;
	p->job_id = job_id;
	p->slot = slot;
	p->frames_h = frames_handle;
	p->frames = frames;
	slot_rec[slot] = rec;
	hold_pending[slot] = 0;
	held_bytes += PG_VSIZE;
}

void pg_rec_free(int rec)
{
	if (rec >= 0 && rec < PG_PROC_MAX && recs[rec].state == REC_BUILT)
		recs[rec].state = REC_FREE;
}

/* Reclaim: called from another job's trap or yield (never the dead job's own context).
 * Returns 0 when the slot holds no address space or it was released; PG_ERR_ACTIVE when the
 * directory is active, in which case the caller keeps the slot dead and retries later. */
int pg_release_owner(unsigned int job_id, int slot)
{
	int r;
	struct pg_rec *p;

	if (slot < 0 || slot >= JOB_MAX || slot_rec[slot] < 0)
		return 0;
	r = slot_rec[slot];
	p = &recs[r];
	if (p->state != REC_LIVE || p->job_id != job_id)
		return 0;
	if (p->dir == cur_cr3 || p->dir == pg_read_cr3()) {
		/* Never free a directory that is active.  Not reachable by construction (the
		 * reclaiming job runs in its own directory); kept as a guard and reported. */
		con_puts("PAGING: release deferred\n");
		return PG_ERR_ACTIVE;
	}
	if (app_frames_release(p->frames_h, PG_OWNER_BASE + (unsigned int) r) != 0)
		con_puts("PAGING: frame release refused\n");
	slot_rec[slot] = -1;
	hold_pending[slot] = 0;
	held_bytes -= PG_VSIZE;
	p->state = REC_FREE;
	p->job_id = 0;
	p->slot = -1;
	return 0;
}

/* QT_PG_CANCEL: remove a paged job that was started held and has never been dispatched.  Releases
 * its frames and record (pg_release_owner), its stack and any heap it owns (job_cancel), and the
 * job id goes stale.  Anything else (not paged, already active, ever dispatched) is refused
 * and left untouched, so a mistaken id cannot hurt a running job. */
int pg_cancel_held(unsigned int job_id)
{
	unsigned int flags;
	int js, rc;

	flags = irq_save();
	js = job_lookup(job_id);
	if (js == JOB_NONE) {
		irq_restore(flags);
		return -2;                       /* QD_ERR_NOJOB */
	}
	if (!pg_slot_paged(js) || jobs[js].prio_inc != 0 || jobs[js].dispatches != 0 ||
	    js == job_current) {
		irq_restore(flags);
		return PG_ERR_NOTHELD;
	}
	rc = pg_release_owner(job_id, js);
	if (rc != 0) {
		irq_restore(flags);
		return rc;
	}
	job_cancel(job_id);
	irq_restore(flags);
	return 0;
}

unsigned int pg_held_bytes(void)
{
	return held_bytes;
}

static int records_live(void)
{
	int r, n = 0;

	for (r = 0; r < PG_PROC_MAX; r++)
		if (recs[r].state != REC_FREE)
			n++;
	return n;
}

/* PTE that maps va in directory d (0 if absent).  Tables are identity-mapped. */
static unsigned int pte_of(unsigned int d, unsigned int va)
{
	unsigned int pde = ((unsigned int *) d)[va >> 22];

	if (!(pde & PG_P))
		return 0;
	return ((unsigned int *) (pde & ~(PG_PAGE - 1u)))[(va >> 12) & 1023u];
}

int pg_caps(void)
{
	unsigned int windows = (PG_POOL_HI - PG_POOL_LO) / PG_VSIZE;
	unsigned int spaces = windows < (unsigned int) PG_PROC_MAX ? windows : (unsigned int) PG_PROC_MAX;

	return (int) (((unsigned int) PG_ABI_VERSION << 24) | (spaces << 16) |
	              PG_CAP_START | PG_CAP_INFO | PG_CAP_ANYEXC | PG_CAP_HOLD | PG_CAP_TASKS | PG_CAP_HELD);
}

#ifdef QDOS_TEST_PG_TASKS
/*
 * TEST ONLY.  Kernel functions registered as poll-list and schedule-list tasks (a kernel function never
 * goes away, so the missing removal service is not an issue).  Each call records the directory it runs
 * under.  Expected: always the template; counted separately while the current job is a paged one.
 * Counters (QT_PG_TEST index): 0 poll calls, 1 poll calls with a paged job current, 2 poll calls under a
 * directory other than the template, 3 schedule calls, 4 .. with a paged job current, 5 .. under another dir.
 */
static struct qtask tt_poll_node, tt_sched_node;
static volatile unsigned int tt_cnt[6];

static void tt_record(int base)
{
	tt_cnt[base]++;
	if (pg_slot_paged(job_current))
		tt_cnt[base + 1]++;
	if (pg_read_cr3() != tpl_dir)
		tt_cnt[base + 2]++;
}

static void tt_poll(void)
{
	tt_record(0);
}

static void tt_sched(void)
{
	tt_record(3);
}

void pg_test_tasks_register(void)
{
	tt_poll_node.fn = tt_poll;
	tt_sched_node.fn = tt_sched;
	qtrap(QT_POLL_ADD, (unsigned int) &tt_poll_node, 0, 0);
	qtrap(QT_SCHED_ADD, (unsigned int) &tt_sched_node, 0, 0);
}

int pg_test_value(unsigned int idx)
{
	return idx < 6u ? (int) tt_cnt[idx] : -1;
}
#endif

int pg_info_fill(unsigned int id, unsigned int user_addr)
{
	unsigned int w[PG_INFO_WORDS], *out, i, cnt = 0, bytes = 0;
	int slot;

	if (!chan_user_range_ok(user_addr, PG_INFO_WORDS * 4u))
		return -2;
	slot = (id == 0) ? job_current : job_lookup(id);
	if (slot == JOB_NONE)
		return -2;
	app_pool_stats(&cnt, &bytes);
	w[0] = pg_slot_paged(slot) ? 1u : 0u;
	w[1] = dir_of_slot(slot);
	w[2] = w[0] ? recs[slot_rec[slot]].frames : 0u;
	w[3] = w[0] ? PG_VSIZE : 0u;
	w[4] = PG_VBASE;
	w[5] = PG_VSIZE;
	w[6] = held_bytes;
	w[7] = cr3_loads;
	w[8] = (unsigned int) jobs[slot].preempts;
	w[9] = (unsigned int) jobs[slot].dispatches;
	w[10] = cnt;
	w[11] = bytes;
	w[12] = pte_of(dir_of_slot(slot), PG_VBASE);
	w[13] = pg_read_cr3();
	w[14] = holds_deferred;
	w[15] = (unsigned int) records_live();
	w[16] = (unsigned int) (irq_spurious_master7 + irq_genuine_master7);
	w[17] = PG_POOL_LO;
	out = (unsigned int *) user_addr;
	for (i = 0; i < PG_INFO_WORDS; i++)
		out[i] = w[i];
	return 0;
}

/*
 * Exception entry.  Frame (lowest first): pushal (edi esi ebp esp ebx edx ecx eax),
 * vector, error code (0 if the CPU pushed none), eip, cs, eflags.
 *
 * R11K: an exception raised at an EIP inside a paged job's own window ends THAT job (the report says
 * so; its exit status is QD_ERR_NOSTATUS) and every other job continues.  Faults at any other EIP
 * (kernel code, unpaged jobs) halt the machine, as before.  NMI, double fault and machine check are
 * never treated as a job's own fault.  The report uses only con_putc on the console, which every
 * directory maps.  The first line keeps the R11J format.
 */
static const char *const exc_name[20] = {
	"#DE divide error", "#DB debug", "NMI", "#BP breakpoint", "#OF overflow",
	"#BR bound range", "#UD invalid opcode", "#NM no x87", "#DF double fault", "coprocessor overrun",
	"#TS invalid TSS", "#NP segment not present", "#SS stack fault", "#GP general protection",
	"#PF page fault", "reserved", "#MF x87 error", "#AC alignment", "#MC machine check", "#XM SIMD"
};

static int exc_job_fault(unsigned int vec)
{
	return vec < 20u && vec != 2u && vec != 8u && vec != 15u && vec != 18u;
}

static void put_reg(const char *name, unsigned int v)
{
	con_puts(name);
	put_hex(v);
}

unsigned int pg_exception(unsigned int sp)
{
	unsigned int *f = (unsigned int *) sp;
	unsigned int vec = f[8], err = f[9], eip = f[10], cr2 = pg_read_cr2();
	int slot = job_current;

	if (exc_depth != 0) {
		con_puts("PGFAULT: nested\n");
		halt_with(0x7f);
	}
	exc_depth = 1;
	con_puts("PGFAULT: vec ");
	put_hex(vec);
	con_puts(" err ");
	put_hex(err);
	con_puts(" cr2 ");
	put_hex(cr2);
	con_puts(" eip ");
	put_hex(eip);
	con_puts(" job ");
	put_hex(jobs[slot].id);
	con_puts(" cr3 ");
	put_hex(pg_read_cr3());
	con_puts("\n");
	con_puts("PGFAULT: ");
	con_puts(vec < 20u ? exc_name[vec] : "unknown");
	put_reg(" eax ", f[7]);
	put_reg(" ecx ", f[6]);
	put_reg(" edx ", f[5]);
	put_reg(" ebx ", f[4]);
	put_reg(" ebp ", f[2]);
	put_reg(" esi ", f[1]);
	put_reg(" edi ", f[0]);
	con_puts("\n");
	/* R11K review: the window addresses 0x240000.. also hold UNPAGED slot-1 code (and poll/schedule tasks
	 * run under the template directory), so a fault is the paged job's own only if its directory is the
	 * one that was active. */
	if (pg_slot_paged(slot) && pg_read_cr3() == dir_of_slot(slot) && in_window(eip) && exc_job_fault(vec)) {
		con_puts("PGFAULT: paged job ended\n");
		exc_depth = 0;
		return job_kill_current(sp);
	}
	con_puts("PGFAULT: halt\n");
	halt_with(0x7e);
	return sp | 1u;
}
