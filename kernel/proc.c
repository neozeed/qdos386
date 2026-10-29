/*
 * QDOS/386 R12A - dynamic processes.  See proc.h, docs/R12A-MEMORY-MAP.md and docs/R12A-INTERRUPT-STACKS.md.
 * Compiled only into -DQDOS_DYNPROC images; replaces kernel/paging.c there and provides the same pg_* entry points.
 *
 * Directory rule (vas.h): pool memory - process directories, tables and frames - is touched only under the template
 * directory, inside tpl_enter()/tpl_leave(), on a kernel stack.  Inside such a section the code never touches the
 * caller's own memory (its trap frame is on its private user stack).
 */
#include "proc.h"
#include "paging.h"
#include "jobs.h"
#include "memory.h"
#include "platform.h"
#include "channel.h"
#include "irq.h"
#include "imgload.h"
#ifdef QDOS_VCON
#include "vcon.h"
#endif
#ifdef QDOS_JOBCTL
#include "jobctl.h"
#endif

extern unsigned int irq_save(void);
extern void irq_restore(unsigned int flags);
extern void outb(unsigned int port, unsigned int val);
extern unsigned int inb(unsigned int port);
unsigned int job_kill_current(unsigned int sp);          /* jobs.c */
void kx_call_user(unsigned int entry, unsigned int user_top, unsigned int *kx_entry_slot);   /* kctx.s */

#define TPL_DIRS   4u                                   /* identity map 0 .. 16 MiB */

/* Template directory and tables: kernel BSS, identity-mapped everywhere. */
static unsigned char tpl_mem[(1 + TPL_DIRS + 1) * VAS_PAGE];
static unsigned int tpl_dir;
static unsigned int tpl_tab[TPL_DIRS];

static struct fr_pool pool;
static unsigned int ram_top;
static unsigned int pool_bad;                           /* frames that failed the boot pattern test */

#define P_FREE   0
#define P_BUILD  1
#define P_LIVE   2

struct proc {
	int state;
	unsigned int id;                 /* tagged job id */
	unsigned int parent;             /* creator's job id (0: kernel) */
	struct vas vas;
	unsigned int origin, win_end;    /* the image's historical window */
	unsigned int img_end;            /* origin + text + data + bss */
	unsigned int img_map_end;        /* page-rounded */
	unsigned int args_page;
	unsigned int entry;
	struct pheap heap;
	unsigned int stk_lo, stk_hi;
	unsigned char hold_pending;
	unsigned int faults;
};

static struct proc procs[JOB_MAX];
static unsigned int cur_cr3;
static unsigned int cr3_loads;
static unsigned int holds_deferred;
static int exc_depth;
static unsigned long procs_created, procs_destroyed, load_failures;

/* ---- template sections ------------------------------------------------------------------------------------------ */

static unsigned int tpl_enter(void)
{
	unsigned int was = cur_cr3;

	if (was != tpl_dir) {
		pg_load_cr3(tpl_dir);
		cur_cr3 = tpl_dir;
		cr3_loads++;
	}
	return was;
}

static void tpl_leave(unsigned int was)
{
	if (was != cur_cr3) {
		pg_load_cr3(was);             /* also flushes the TLB of the caller's space after a mapping change */
		cur_cr3 = was;
		cr3_loads++;
	}
}

unsigned int pg_enter_template(void)
{
	return tpl_enter();
}

void pg_leave_template(unsigned int was)
{
	tpl_leave(was);
}

/* ---- boot ------------------------------------------------------------------------------------------------------- */

static void put_hex(unsigned int v)
{
	static const char hx[] = "0123456789abcdef";
	int s;

	for (s = 28; s >= 0; s -= 4)
		con_putc((unsigned char) hx[(v >> s) & 15u]);
}

static unsigned int cmos_read(unsigned int reg)
{
	outb(0x70, reg);
	return inb(0x71) & 0xFFu;
}

/* Add [lo, hi) after a presence test of every frame (three words written and read back). */
static void pool_add_tested(unsigned int lo, unsigned int hi)
{
	unsigned int a;
	volatile unsigned int *w;

	if (hi > ram_top)
		hi = ram_top;
	if (hi <= lo)
		return;
	fr_add_range(&pool, lo, hi);
	for (a = lo; a < hi; a += VAS_PAGE) {
		w = (volatile unsigned int *) a;
		w[0] = a ^ 0x5AA55AA5u;
		w[512] = ~a;
		w[1023] = a ^ 0xC3C3C3C3u;
		if (w[0] != (a ^ 0x5AA55AA5u) || w[512] != ~a || w[1023] != (a ^ 0xC3C3C3C3u)) {
			fr_remove(&pool, a);
			pool_bad++;
		}
	}
}

static int shell_is_process = 0;

void proc_boot(void)
{
	unsigned int i, ext;

	tpl_dir = ((unsigned int) tpl_mem + VAS_PAGE - 1u) & ~(VAS_PAGE - 1u);
	for (i = 0; i < TPL_DIRS; i++)
		tpl_tab[i] = tpl_dir + (1u + i) * VAS_PAGE;
	vas_template_build(tpl_dir, tpl_tab, TPL_DIRS);
	for (i = 0; i < (unsigned int) JOB_MAX; i++)
		procs[i].state = P_FREE;
	pg_load_cr3(tpl_dir);
	cur_cr3 = tpl_dir;
	pg_enable();
	/* RAM: 1 MiB + CMOS 0x30/0x31 KiB (QEMU/SeaBIOS and Tiny386 both fill it), capped at the 16 MiB identity map. */
	ext = cmos_read(0x30) | (cmos_read(0x31) << 8);
	ram_top = 0x00100000u + ext * 1024u;
	if (ext == 0 || ram_top > FR_LIMIT)
		ram_top = (ext == 0) ? 0x00800000u : FR_LIMIT;
	fr_init(&pool);
#ifdef QDOS_DYNPROC_SHELL
	shell_is_process = 1;
#endif
	if (shell_is_process)
		pool_add_tested(APP_SLOT0_BASE, APP_SLOT1_BASE);       /* the former shell slot */
	pool_add_tested(APP_SLOT1_BASE, APP_SLOT1_BASE + APP_SLOT1_SIZE);   /* former slot 1 */
	pool_add_tested(APP_PROFILE2_BASE, 0x00700000u);        /* former profiles 2/3, up to the boot stack area */
	pool_add_tested(0x00800000u, FR_LIMIT);                 /* above the boot stack, as far as RAM goes */
	{
		extern void kx_tss_init(unsigned int template_cr3);   /* descriptors.c */
		kx_tss_init(tpl_dir);
	}
	con_puts("PAGING: R12A dynamic processes, ram ");
	con_putdec((unsigned long) (ram_top / 1024u));
	con_puts(" KiB, pool ");
	con_putdec((unsigned long) pool.pool);
	con_puts(" frames");
	if (pool_bad != 0) {
		con_puts(" (");
		con_putdec((unsigned long) pool_bad);
		con_puts(" failed test)");
	}
	con_puts(", template cr3 ");
	put_hex(tpl_dir);
	con_puts("\n");
}

/* Compatibility name used by shellboot.c. */
void pg_init(void)
{
	proc_boot();
}

/* ---- scheduler hooks -------------------------------------------------------------------------------------------- */

static unsigned int dir_of_slot(int slot)
{
	if (slot >= 0 && slot < JOB_MAX && procs[slot].state == P_LIVE)
		return procs[slot].vas.dir;
	return tpl_dir;
}

void pg_activate(int slot)
{
	unsigned int d = dir_of_slot(slot);

	if (d != cur_cr3) {
		pg_load_cr3(d);
		cur_cr3 = d;
		cr3_loads++;
	}
}

int pg_slot_paged(int slot)
{
	return slot >= 0 && slot < JOB_MAX && procs[slot].state == P_LIVE;
}

/* A process's hold flag is in its private memory (pg_user_range_ok admits nothing else), so it is written only while
 * that process's directory is active; otherwise the write is deferred to pg_after_switch.  Flags of template jobs
 * (SYSTEM, IDLE, an unpaged shell) are in identity-mapped memory and are written at once. */
void pg_hold_release(int slot)
{
	struct job *j = &jobs[slot];

	if (j->hold == 0)
		return;
	if (pg_slot_paged(slot) && dir_of_slot(slot) != cur_cr3) {
		procs[slot].hold_pending = 1;
		holds_deferred++;
		return;
	}
	*j->hold = 0xff;
}

void pg_after_switch(int slot)
{
	if (slot < 0 || slot >= JOB_MAX || !procs[slot].hold_pending)
		return;
	procs[slot].hold_pending = 0;
	if (jobs[slot].hold != 0 && dir_of_slot(slot) == cur_cr3)
		*jobs[slot].hold = 0xff;
}

/* ---- user pointers ---------------------------------------------------------------------------------------------- */

static int inside(unsigned int addr, unsigned int end, unsigned int lo, unsigned int hi)
{
	return addr >= lo && end <= hi;
}

/* 1 ok, 0 refused, -1 the caller is not a process (channel.c then applies the flat R11E check). */
int pg_user_range_ok(unsigned int addr, unsigned int n)
{
	struct proc *p;
	unsigned int end;

	if (!pg_slot_paged(job_current))
		return -1;
	p = &procs[job_current];
	if (addr == 0)
		return 0;
	end = addr + n;
	if (end < addr)
		return 0;
	return inside(addr, end, p->origin, p->img_map_end) ||
	       inside(addr, end, p->args_page, p->args_page + VAS_PAGE) ||
	       inside(addr, end, p->heap.base, p->heap.brk) ||
	       inside(addr, end, p->stk_lo, p->stk_hi);
}

/* ---- creation --------------------------------------------------------------------------------------------------- */

static void wr32(unsigned char *p, unsigned int v)
{
	p[0] = (unsigned char) v;
	p[1] = (unsigned char) (v >> 8);
	p[2] = (unsigned char) (v >> 16);
	p[3] = (unsigned char) (v >> 24);
}

static int args_count(const unsigned char *packed, unsigned int plen)
{
	unsigned int i, argc = 0;

	if (plen == 0)
		return 0;
	if (plen > APP_ARG_STR_MAX || packed[plen - 1] != 0)
		return -1;
	for (i = 0; i < plen; i++)
		if (packed[i] == 0) {
			if (argc == APP_ARG_MAX)
				return -1;
			argc++;
		}
	return (int) argc;
}

static unsigned char ld_buf[512];
static unsigned char arg_blk[APP_ARG_BYTES];

static void proc_entry(void)
{
	struct proc *p = &procs[job_current];

	kx_call_user(p->entry, PROC_STACK_TOP, &jobs[job_current].kx_entry);
}

static unsigned int heap_grow(void *ctx, unsigned int brk, unsigned int want);

/* Build everything inside one template section.  Returns VAS_OK or a negative code; the caller undoes on failure. */
static int build_space(struct proc *p, app_rd_fn rd, void *ctx, const struct app_image *info, unsigned int argc,
                       const unsigned char *packed)
{
	unsigned int va, off, rem, n, i, k, p0;
	int rc;

	rc = vas_create(&p->vas, &pool, tpl_dir, TPL_DIRS);
	if (rc != VAS_OK)
		return rc;
	if ((rc = vas_hide(&p->vas, 0)) != VAS_OK)          /* page 0 not present: a NULL dereference faults */
		return rc;
	for (va = p->origin; va < p->img_map_end; va += VAS_PAGE)
		if ((rc = vas_map_new(&p->vas, &pool, va, VAS_RW | VAS_US)) != VAS_OK)
			return rc;
	if (p->args_page >= p->img_map_end)
		if ((rc = vas_map_new(&p->vas, &pool, p->args_page, VAS_RW | VAS_US)) != VAS_OK)
			return rc;
	for (va = p->stk_lo; va < p->stk_hi; va += VAS_PAGE)
		if ((rc = vas_map_new(&p->vas, &pool, va, VAS_RW | VAS_US)) != VAS_OK)
			return rc;
	/* text and data, read in 512-byte chunks; BSS is already zero (frames are zeroed when mapped) */
	off = AOUT_HDR_BYTES;
	va = p->origin;
	rem = info->text + info->data;
	while (rem != 0) {
		n = rem < sizeof ld_buf ? rem : (unsigned int) sizeof ld_buf;
		rc = rd(ctx, off, ld_buf, n);
		if (rc != APP_OK)
			return rc;
		if (vas_copy_to(&p->vas, va, ld_buf, n) != VAS_OK)
			return APP_ERR_LAYOUT;
		off += n;
		va += n;
		rem -= n;
	}
	/* argument block (R8 layout) with pointers for its virtual address */
	for (i = 0; i < APP_ARG_BYTES; i++)
		arg_blk[i] = 0;
	p0 = p->args_page + VAS_PAGE - APP_ARG_BYTES;
	wr32(arg_blk, argc);
	wr32(arg_blk + 4, p0 + 8);
	k = 0;
	n = 0;
	for (i = 0; i < argc; i++) {
		wr32(arg_blk + 8 + 4 * i, p0 + APP_ARG_STR_OFF + k);
		while (packed[n] != 0)
			arg_blk[APP_ARG_STR_OFF + k++] = packed[n++];
		arg_blk[APP_ARG_STR_OFF + k++] = 0;
		n++;
	}
	wr32(arg_blk + 8 + 4 * argc, 0);
	if (vas_copy_to(&p->vas, p0, arg_blk, APP_ARG_BYTES) != VAS_OK)
		return APP_ERR_LAYOUT;
	return VAS_OK;
}

int proc_shell_is_process(void)
{
	return shell_is_process;
}

int proc_load(app_rd_fn rd, void *ctx, unsigned int len, int want_slot, const unsigned char *packed,
              unsigned int plen, unsigned int flags)
{
	struct app_image info;
	struct proc *p;
	unsigned int irq, was, top, creator;
	int rc, id, js, argc;

	rc = aout_check_src(rd, ctx, len, &info);           /* reads only */
	if (rc != APP_OK)
		return rc;
	if (want_slot >= 0 && (unsigned int) want_slot != info.slot)
		return (want_slot == 1) ? APP_ERR_NOTSLOT1 : (want_slot == 0) ? APP_ERR_NOTSLOT0 : APP_ERR_NOPROFILE;
	argc = args_count(packed, plen);
	if (argc < 0)
		return APP_ERR_ARGS;
	if (info.slot == 0 && !shell_is_process)
		return APP_ERR_ADDROCC;                          /* the shell slot is not pool memory in this build */
	creator = jobs[job_current].id;
	irq = irq_save();
	id = job_alloc_sized(info.slot == 0 ? "APP0" : "APPD", proc_entry, 0, PROC_KSTACK_BYTES);
	if (id < 0) {
		irq_restore(irq);
		load_failures++;
		return (id == -1) ? APP_ERR_NOJOB : APP_ERR_NOMEM;
	}
	js = job_lookup((unsigned int) id);
	p = &procs[js];
	p->state = P_BUILD;
	p->id = (unsigned int) id;
	p->parent = creator;
	p->origin = info.entry;
	p->win_end = info.entry + APP_PROFILE_SIZE(info.slot);
	p->img_end = info.entry + info.text + info.data + info.bss;
	p->img_map_end = (p->img_end + VAS_PAGE - 1u) & ~(VAS_PAGE - 1u);
	if (p->img_map_end == p->origin)
		p->img_map_end = p->origin + VAS_PAGE;
	p->args_page = p->win_end - VAS_PAGE;
	p->entry = info.entry;
	p->stk_hi = PROC_STACK_TOP;
	p->stk_lo = PROC_STACK_TOP - PROC_STACK_BYTES;
	p->hold_pending = 0;
	p->faults = 0;
	p->vas.dir = 0;
	p->vas.ntab = 0;
	p->vas.owned = 0;
	ph_init(&p->heap, PROC_HEAP_BASE, PROC_HEAP_BASE + PROC_HEAP_MAX, (unsigned int) id, heap_grow, p);
	was = tpl_enter();
	rc = build_space(p, rd, ctx, &info, (unsigned int) argc, packed);
	if (rc != VAS_OK)
		vas_destroy(&p->vas, &pool);                    /* every frame, table and the directory */
	tpl_leave(was);
	if (rc != VAS_OK) {
		p->state = P_FREE;
		job_cancel((unsigned int) id);                  /* kernel stack, job record */
		irq_restore(irq);
		load_failures++;
		return rc == VAS_ERR_NOFRAMES ? PROC_ERR_NOFRAMES : (rc == VAS_ERR_ARG || rc == VAS_ERR_TABLES) ? PROC_ERR_ORIGIN : rc;
	}
	top = jobs[js].stack_top;
	jobs[js].args = p->args_page + VAS_PAGE - APP_ARG_BYTES;
	jobs[js].kx_lo = jobs[js].stack_base;
	jobs[js].kx_hi = top;
	jobs[js].kx_entry = (top - 64u) & ~15u;
	p->state = P_LIVE;
	procs_created++;
	jobs[js].prio_inc = (flags & PROC_LOAD_HELD) ? 0 : 2;
	irq_restore(irq);
	return id;
}

/* Reclaim (another job's trap or yield).  0: no space or released; PG_ERR_ACTIVE: its directory is active (guard). */
int pg_release_owner(unsigned int job_id, int slot)
{
	struct proc *p;
	unsigned int was;

	if (slot < 0 || slot >= JOB_MAX)
		return 0;
	p = &procs[slot];
	if (p->state != P_LIVE || p->id != job_id)
		return 0;
	if (p->vas.dir == cur_cr3 || p->vas.dir == pg_read_cr3()) {
		con_puts("PAGING: release deferred\n");
		return PG_ERR_ACTIVE;
	}
	was = tpl_enter();
	vas_destroy(&p->vas, &pool);
	tpl_leave(was);
	p->state = P_FREE;
	p->hold_pending = 0;
	p->id = 0;
	procs_destroyed++;
	return 0;
}

int pg_cancel_held(unsigned int job_id)
{
	unsigned int flags;
	int js, rc;

	flags = irq_save();
	js = job_lookup(job_id);
	if (js == JOB_NONE) {
		irq_restore(flags);
		return -2;
	}
	if (!pg_slot_paged(js) || jobs[js].prio_inc != 0 || jobs[js].dispatches != 0 || js == job_current) {
		irq_restore(flags);
		return PG_ERR_NOTHELD;
	}
	rc = pg_release_owner(job_id, js);
	if (rc == 0)
		job_cancel(job_id);
	irq_restore(flags);
	return rc;
}

/* ---- private heap ----------------------------------------------------------------------------------------------- */

/* Grow callback: map frames for [brk, want) one page at a time; stop at the first failure.  The allocator only keeps
 * what was really mapped.  Runs in the owner's own trap; the template section ends before returning, so the caller is
 * back in the owner's directory (and its TLB flushed) when it writes the new free block. */
static unsigned int heap_grow(void *ctx, unsigned int brk, unsigned int want)
{
	struct proc *p = (struct proc *) ctx;
	unsigned int va, was;

	was = tpl_enter();
	for (va = brk; va < want; va += VAS_PAGE)
		if (vas_map_new(&p->vas, &pool, va, VAS_RW | VAS_US) != VAS_OK)
			break;
	tpl_leave(was);
	return va;
}

int proc_mem_alloc(unsigned int bytes)
{
	return (int) ph_alloc(&procs[job_current].heap, bytes);
}

int proc_mem_free(unsigned int ptr)
{
	return ph_free(&procs[job_current].heap, ptr, procs[job_current].id);
}

int proc_mem_validate(void)
{
	if (mem_validate() != MEM_OK)
		return MEM_ERR_CORRUPT;
	return ph_validate(&procs[job_current].heap) == PH_OK ? MEM_OK : MEM_ERR_CORRUPT;
}

/* ---- parent/child records (Stage 3) -----------------------------------------------------------------------------
 * A record is made only by QT_PROC_SPAWN (the shell's RUN/START veneers keep the R11M semantics and make none).  It
 * holds the child's exit status after the child is gone, until the parent waits for it (zombie retention) or the parent
 * itself ends (the records are dropped; a running child goes on as an orphan and nobody collects its status).  Records
 * live in kernel memory; a wait never writes into another process's space: a blocked QT_PROC_WAIT returns
 * PROC_ERR_AGAIN when woken and libqdos repeats the call, which then finds the status. */
#define CREC_MAX     32
#define CREC_FREE    0
#define CREC_RUN     1
#define CREC_ENDED   2

struct crec {
	int state;
	unsigned int parent, child;
	int status;
	int waiting;             /* the parent sleeps in QT_PROC_WAIT for this child */
};
static struct crec crecs[CREC_MAX];
static unsigned long spawns, waits_blocked, orphans;

static int children_of(unsigned int parent)
{
	int i, n = 0;

	for (i = 0; i < CREC_MAX; i++)
		if (crecs[i].state != CREC_FREE && crecs[i].parent == parent)
			n++;
	return n;
}

static struct crec *crec_find(unsigned int parent, unsigned int child)
{
	int i;

	for (i = 0; i < CREC_MAX; i++)
		if (crecs[i].state != CREC_FREE && crecs[i].parent == parent && crecs[i].child == child)
			return &crecs[i];
	return 0;
}

int job_status_of(unsigned int id);         /* jobs.c: QT_JOB_EXIT body */
int img_spawn_request(unsigned int name_addr, unsigned int args_addr, unsigned int args_len, unsigned int flags);

/* Reclaim hook (jobs.c reclaim_dead, before the slot is freed): record a child's end and wake a waiting parent; drop
 * the records of a parent that is going away. */
void proc_job_reaped(unsigned int id, int status)
{
	int i, ps;

	for (i = 0; i < CREC_MAX; i++) {
		if (crecs[i].state == CREC_FREE)
			continue;
		if (crecs[i].parent == id) {
			if (crecs[i].state == CREC_RUN)
				orphans++;
			crecs[i].state = CREC_FREE;
			continue;
		}
		if (crecs[i].child == id && crecs[i].state == CREC_RUN) {
			crecs[i].state = CREC_ENDED;
			crecs[i].status = status;
			if (crecs[i].waiting) {
				crecs[i].waiting = 0;
				ps = job_lookup(crecs[i].parent);
				if (ps != JOB_NONE && jobs[ps].status != 0 && !jobs[ps].dead) {
					jobs[ps].status = 0;          /* runnable; no hold flag is involved */
					jobs[ps].hold = 0;
				}
			}
		}
	}
}

int proc_spawn(unsigned int name_addr, unsigned int req_addr)
{
	unsigned int req[5], i, me = jobs[job_current].id;
	int id, slot = -1, con;

	if (!chan_user_range_ok(req_addr, sizeof req))
		return CHAN_ERR_RANGE;
	for (i = 0; i < 5u; i++)
		req[i] = ((unsigned int *) req_addr)[i];
	con = (int) req[4];
	if (req[0] != 1u || (req[3] & ~PROC_LOAD_HELD) != 0 || con < -1 || con > 3)
		return APP_ERR_ARGS;
	if (children_of(me) >= PROC_CHILD_LIMIT)
		return PROC_ERR_LIMIT;
	for (i = 0; i < CREC_MAX; i++)
		if (crecs[i].state == CREC_FREE) {
			slot = (int) i;
			break;
		}
	if (slot < 0)
		return PROC_ERR_LIMIT;
#ifdef QDOS_VCON
	if (con >= 0 && vcon_set_child(job_current, con) != 0)
		return APP_ERR_ARGS;
#endif
	id = img_spawn_request(name_addr, req[1], req[2], req[3]);
#ifdef QDOS_VCON
	if (con >= 0)
		(void) vcon_set_child(job_current, -1);      /* used or not, the choice is not left pending */
#endif
	if (id < 0)
		return id;
#ifdef QDOS_JOBCTL
	jobctl_note_launch(id, name_addr, JC_F_PAGED);
#endif
	crecs[slot].state = CREC_RUN;
	crecs[slot].parent = me;
	crecs[slot].child = (unsigned int) id;
	crecs[slot].status = 0;
	crecs[slot].waiting = 0;
	spawns++;
	return id;
}

/* *block is set when the caller must sleep: jobs.c then answers PROC_ERR_AGAIN and schedules. */
int proc_wait(unsigned int id, int timeout, int *block)
{
	unsigned int me = jobs[job_current].id;
	struct crec *c = crec_find(me, id);
	int js, st;

	*block = 0;
	if (c == 0)
		return PROC_ERR_NOTCHILD;
	c->waiting = 0;
	if (c->state == CREC_RUN) {
		js = job_lookup(id);
		if (js == JOB_NONE || jobs[js].dead) {
			c->state = CREC_ENDED;                  /* ended, not yet reclaimed: the status is already final */
			c->status = job_status_of(id);
		}
	}
	if (c->state == CREC_ENDED) {
		st = c->status;
		c->state = CREC_FREE;                       /* reaped */
		return st;
	}
	if (timeout == 0)
		return PROC_ERR_TIMEOUT;
	c->waiting = 1;
	jobs[job_current].status = timeout < 0 ? -1 : timeout;
	jobs[job_current].hold = 0;
	waits_blocked++;
	*block = 1;
	return PROC_ERR_AGAIN;
}

int proc_kill(unsigned int id)
{
	struct crec *c = crec_find(jobs[job_current].id, id);
	int js, rc;

	if (c == 0)
		return PROC_ERR_NOTCHILD;
	if (c->state != CREC_RUN)
		return 0;                                   /* already ended: nothing to do, the status stays */
	js = job_lookup(id);
	if (js != JOB_NONE && jobs[js].prio_inc == 0 && jobs[js].dispatches == 0 && !jobs[js].dead) {
		rc = pg_cancel_held(id);                    /* held, never ran: released at once */
		if (rc == 0) {
			c->state = CREC_ENDED;
			c->status = -66;                        /* QD_ERR_KILLED */
		}
		return rc;
	}
#ifdef QDOS_JOBCTL
	rc = jobctl_kill(id);
	return rc == JC_ERR_DEAD ? 0 : rc;
#else
	return PROC_ERR_REFUSED;
#endif
}

/* ---- information ------------------------------------------------------------------------------------------------ */

static int procs_live(void)
{
	int i, n = 0;

	for (i = 0; i < JOB_MAX; i++)
		if (procs[i].state == P_LIVE)
			n++;
	return n;
}

int pg_caps(void)
{
	return (int) ((3u << 24) | ((unsigned int) (JOB_MAX - 2) << 16) |
	              PG_CAP_START | PG_CAP_INFO | PG_CAP_ANYEXC | PG_CAP_HOLD | PG_CAP_TASKS | PG_CAP_HELD);
}

int proc_caps(void)
{
	return (int) (((unsigned int) PROC_ABI_VERSION << 24) | ((unsigned int) JOB_MAX << 16) |
	              PROC_CAP_DYNAMIC | PROC_CAP_PHEAP | PROC_CAP_PSTACK | PROC_CAP_GUARD | PROC_CAP_SPAWN);
}

/* QT_PG_INFO with the R11K layout; contents for a dynamic space (frame_base = frame of the first image page). */
int pg_info_fill(unsigned int id, unsigned int user_addr)
{
	unsigned int w[PG_INFO_WORDS], *out, i, was, pte = 0;
	int slot;
	struct proc *p;

	if (!chan_user_range_ok(user_addr, PG_INFO_WORDS * 4u))
		return -2;
	slot = (id == 0) ? job_current : job_lookup(id);
	if (slot == JOB_NONE)
		return -2;
	p = &procs[slot];
	if (p->state == P_LIVE) {
		was = tpl_enter();
		pte = vas_lookup(&p->vas, p->origin);
		tpl_leave(was);
	}
	w[0] = p->state == P_LIVE ? 1u : 0u;
	w[1] = dir_of_slot(slot);
	w[2] = w[0] ? (pte & ~(VAS_PAGE - 1u)) : 0u;
	w[3] = w[0] ? vas_frames(&p->vas) * VAS_PAGE : 0u;
	w[4] = w[0] ? p->origin : APP_SLOT1_BASE;
	w[5] = w[0] ? p->win_end - p->origin : APP_SLOT1_SIZE;
	w[6] = pool.nused * VAS_PAGE;
	w[7] = cr3_loads;
	w[8] = (unsigned int) jobs[slot].preempts;
	w[9] = (unsigned int) jobs[slot].dispatches;
	w[10] = (unsigned int) procs_live();
	w[11] = pool.nused * VAS_PAGE;
	w[12] = pte;
	w[13] = pg_read_cr3();
	w[14] = holds_deferred;
	w[15] = (unsigned int) procs_live();
	w[16] = (unsigned int) (irq_spurious_master7 + irq_genuine_master7);
	w[17] = APP_SLOT1_BASE;
	out = (unsigned int *) user_addr;
	for (i = 0; i < PG_INFO_WORDS; i++)
		out[i] = w[i];
	return 0;
}

static unsigned int kstack_hwm(const struct job *j)
{
	unsigned int a;

	for (a = j->stack_base + 16u; a < j->stack_top; a += 4u)
		if (*(unsigned int *) a != 0)
			return j->stack_top - a;
	return 0;
}

int proc_info(unsigned int id, unsigned int user_addr)
{
	unsigned int w[PROC_INFO_WORDS], *out, i;
	int slot;
	struct proc *p;

	if (!chan_user_range_ok(user_addr, PROC_INFO_WORDS * 4u))
		return CHAN_ERR_RANGE;
	slot = (id == 0) ? job_current : job_lookup(id);
	if (slot == JOB_NONE)
		return -2;
	p = &procs[slot];
	for (i = 0; i < PROC_INFO_WORDS; i++)
		w[i] = 0;
	w[0] = PROC_ABI_VERSION;
	w[1] = jobs[slot].id;
	w[3] = p->state == P_LIVE ? 1u : 0u;
	w[4] = dir_of_slot(slot);
	if (p->state == P_LIVE) {
		w[2] = p->parent;
		w[5] = p->origin;
		w[6] = p->img_end;
		w[7] = jobs[slot].args;
		w[8] = p->heap.base;
		w[9] = p->heap.brk;
		w[10] = p->heap.cap;
		w[11] = p->heap.used;
		w[12] = p->stk_lo;
		w[13] = p->stk_hi;
		w[14] = vas_frames(&p->vas);
		w[17] = jobs[slot].stack_bytes;
		w[18] = kstack_hwm(&jobs[slot]);
	}
	w[15] = pool.pool;
	w[16] = fr_free_count(&pool);
	w[19] = (unsigned int) jobs[slot].dispatches;
	w[20] = (unsigned int) jobs[slot].preempts;
	w[21] = p->faults;
	w[22] = (unsigned int) children_of(jobs[slot].id);
	w[23] = (unsigned int) procs_live();
	out = (unsigned int *) user_addr;
	for (i = 0; i < PROC_INFO_WORDS; i++)
		out[i] = w[i];
	return 0;
}

/* Diagnostic: the PTE that maps va in the caller's own space (0 if not present, or the caller is not a process). */
int proc_va(unsigned int va)
{
	unsigned int was, pte;

	if (!pg_slot_paged(job_current))
		return 0;
	was = tpl_enter();
	pte = vas_lookup(&procs[job_current].vas, va);
	tpl_leave(was);
	return (int) pte;
}

/* ---- exceptions ------------------------------------------------------------------------------------------------- */

static const char *const exc_name[20] = {
	"#DE divide error", "#DB debug", "NMI", "#BP breakpoint", "#OF overflow",
	"#BR bound range", "#UD invalid opcode", "#NM no x87", "#DF double fault", "coprocessor overrun",
	"#TS invalid TSS", "#NP segment not present", "#SS stack fault", "#GP general protection",
	"#PF page fault", "reserved", "#MF x87 error", "#AC alignment", "#MC machine check", "#XM SIMD"
};

extern char bss_start[];               /* kernel.ld: end of kernel text and data */

/* Kernel code is the kernel image, 0x100000 .. bss_start.  A fault at any other EIP while a process's own directory is
 * active was raised by that process's code (its image, or a wild jump/fetch from it), never by the kernel. */
static int kernel_code(unsigned int eip)
{
	return eip >= 0x00100000u && eip < (unsigned int) bss_start;
}

static int exc_job_fault(unsigned int vec)
{
	return vec < 20u && vec != 2u && vec != 8u && vec != 15u && vec != 18u;
}

static void put_reg(const char *name, unsigned int v)
{
	con_puts(name);
	put_hex(v);
}

/*
 * Exception entry (kctx.s, on the job's kernel stack).  Frame (lowest first): pushal, vector, error code, eip, cs,
 * eflags - on the stack that was current when the exception happened (for a process: its user stack, read here under
 * its own directory).  A fault raised by a process's own code (EIP outside the kernel image while its directory is
 * active) ends only that process; anything else halts.  Report: vector, error, CR2, EIP, job id, CR3.
 */
unsigned int pg_exception(unsigned int sp)
{
	unsigned int *f = (unsigned int *) sp;
	unsigned int vec = f[8], err = f[9], eip = f[10], cr2 = pg_read_cr2();
	int slot = job_current;
	struct proc *p = &procs[slot];

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
	con_puts("\nPGFAULT: ");
	con_puts(vec < 20u ? exc_name[vec] : "unknown");
	put_reg(" eax ", f[7]);
	put_reg(" ecx ", f[6]);
	put_reg(" edx ", f[5]);
	put_reg(" ebx ", f[4]);
	put_reg(" ebp ", f[2]);
	put_reg(" esi ", f[1]);
	put_reg(" edi ", f[0]);
	con_puts("\n");
	if (pg_slot_paged(slot) && pg_read_cr3() == p->vas.dir && !kernel_code(eip) && exc_job_fault(vec)) {
		p->faults++;
		con_puts("PGFAULT: process ended\n");
		exc_depth = 0;
		return job_kill_current(sp);
	}
	con_puts("PGFAULT: halt\n");
	halt_with(0x7e);
	return sp | 1u;
}

/*
 * #DF arrives through the task gate (descriptors.c) on its own stack, template directory.  The interrupted state is in
 * the main TSS.  A user-stack overflow of the current process (its directory, ESP at or below the bottom of its stack and
 * within one stack size of it) is ended: the main TSS is rewritten so that the task return resumes in kx_df_kill on the
 * job's kernel stack.  Anything else halts.
 */
extern unsigned int kx_tss_main[26];
extern void kx_df_kill(void);

void proc_double_fault(void)
{
	int slot = job_current;
	struct proc *p = &procs[slot];
	unsigned int cr3 = cur_cr3, eip = kx_tss_main[8], esp = kx_tss_main[14];

	/* A task switch does not store CR3 into the outgoing TSS, but the task return loads it from there: set it to the
	 * directory that was active (the scheduler's bookkeeping), whatever happens next. */
	kx_tss_main[7] = cur_cr3;

	con_puts("PGFAULT: #DF double fault eip ");
	put_hex(eip);
	con_puts(" esp ");
	put_hex(esp);
	con_puts(" cr3 ");
	put_hex(cr3);
	con_puts(" job ");
	put_hex(jobs[slot].id);
	con_puts("\n");
	if (pg_slot_paged(slot) && cr3 == p->vas.dir && esp < p->stk_lo + 64u && esp + PROC_STACK_BYTES >= p->stk_lo) {
		con_puts("PGFAULT: user stack overflow, process ended\n");
		p->faults++;
		kx_tss_main[8] = (unsigned int) kx_df_kill;                 /* eip */
		kx_tss_main[9] = 0x2u;                                      /* eflags: IF off */
		kx_tss_main[14] = jobs[slot].kx_entry;                      /* esp: the job's kernel stack */
		kx_tss_main[15] = 0;                                        /* ebp */
		kx_tss_main[19] = 0x08u;                                    /* cs */
		kx_tss_main[18] = kx_tss_main[20] = kx_tss_main[21] = kx_tss_main[22] = kx_tss_main[23] = 0x10u;
		return;
	}
	con_puts("PGFAULT: halt\n");
	halt_with(0x7d);
}

unsigned int proc_df_kill(unsigned int sp)
{
	return job_kill_current(sp);
}
