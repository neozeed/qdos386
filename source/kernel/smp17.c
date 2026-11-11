/*
 * QDOS/386 R17 Apollo 13 - up to eight CPUs under the Big Kernel Lock, topology discovery and topology-aware
 * foreground-first placement (docs/APOLLO13-TOPOLOGY.md, docs/APOLLO13-AUDIT.md).  Compiled instead of smp.c into the
 * opt-in R17 kernel only; smp.c (R16/R16A/R16B) is unchanged.
 *
 * CPU 0 (the BSP) owns every device interrupt and the timebase.  Each application processor n = 1..7 has its own TSS
 * (selector 0x30 + 8n), kernel stack (R17 kernel area), idle job (halts), LAPIC timer and per-CPU kernel block; it
 * enters the shared kernel only under the BKL.  APs are started one at a time (INIT, SIPI, SIPI) with a barrier: an AP
 * that does not report within the time limit is sent INIT again (held in wait-for-SIPI), its idle job is released and
 * it is never scheduled.  Placement only rewrites jobs[].aff under the BKL; the owning CPU honours a move at its next
 * schedule() pass after the frame and x87 state are saved (docs/R16B-MIGRATION-ABI.md, unchanged).
 */
#include "jobs.h"
#include "platform.h"
#include "channel.h"
#include "bkl.h"
#include "smp.h"
#ifdef QDOS_PAGING
#include "paging.h"
#endif

extern void outb(unsigned int port, unsigned int val);
extern unsigned int inb(unsigned int port);
extern void pg_map_lapic(unsigned int lapic_pa);
extern unsigned int pg_read_cr3(void);
extern unsigned int r17_karea_alloc(unsigned int bytes);   /* proc.c: the R17 kernel area */

/* smp_s17.s */
int smp_has_cpuid(void);
void smp_cpuid(unsigned int leaf, unsigned int *r);
void smp_cpuid2(unsigned int leaf, unsigned int sub, unsigned int *r);
unsigned int smp_rdmsr(unsigned int msr, unsigned int *hi);
unsigned int smp_mmio_rd(unsigned int a);
void smp_mmio_wr(unsigned int a, unsigned int v);
extern unsigned char smp_trampoline[], smp_trampoline_end[];
extern unsigned int smp_ap_pdbr, smp_ap_gdtr, smp_ap_idtr, smp_ap_esp, smp_ap_entered, smp_ap_tr;
void smp_ap_timer_stub(void);
void smp_ap_idle_entry(void);
void smp_ap_main(void);
unsigned int smp_ap_timer_resume(unsigned int sp);
int kx_this_cpu(void);
unsigned int schedule_entry(unsigned int sp);
extern unsigned int irq_save(void);
extern void irq_restore(unsigned int flags);

/* descriptors.c */
extern unsigned int kx_gdtr_addr(void);
extern unsigned int kx_idtr_addr(void);
unsigned int kx_install_ap_tss_cpu(int cpu, unsigned int esp0, unsigned int cr3);
void kx_set_ap_gate(int vector, unsigned int handler);

#define NCPU         KX_NCPU
#define TRAMP_PA     0x00009000u
#define LAPIC_ID     0x020u
#define LAPIC_SVR    0x0F0u
#define LAPIC_EOI    0x0B0u
#define LAPIC_LVTT   0x320u
#define LAPIC_TDCR   0x3E0u
#define LAPIC_TICR   0x380u
#define LAPIC_TCCR   0x390u
#define AP_TIMER_VEC 0x40u
#define AP_STACK     16384u

/* topology methods */
#define TOPO_UNKNOWN 0     /* no CPUID topology information: flat, siblings never invented */
#define TOPO_LEAF0B  1     /* CPUID leaf 0xB (x2APIC topology, Nehalem and later) */
#define TOPO_LEGACY  2     /* CPUID leaf 1 (HTT, logical per package) + leaf 4 (cores per package) */
#define TOPO_BAD     3     /* information present but inconsistent across CPUs or with the MADT: flat */

struct cpu_ent {
	unsigned int apic;          /* LAPIC ID register (8 bit), read on that CPU */
	unsigned int cpuid_apic;    /* APIC id CPUID reports on that CPU (leaf 0xB EDX or leaf 1 EBX[31:24]) */
	unsigned int smt_shift, core_shift, method;
	int pkg, core, thread;
	int online, parked;
	int idle_slot;
	unsigned int stack;
	unsigned long ticks, dispatch;
};

static int state = SMP_S_DOWN, reason = 0, ncpu = 0, nup = 1;
static unsigned int lapic = 0, bsp_id = 0, lvt_count = 0;
static struct cpu_ent cpus[NCPU];
static unsigned int fail_ids[16], nfail = 0;
static int topo = TOPO_UNKNOWN, topo_smt = 0;        /* topo_smt: proven SMT siblings exist */
static unsigned int ht_mode = SMP_HT_ALL;
static volatile unsigned int ap_ready = 0;
static const char *idle_names[NCPU] = { "IDLE", "IDL1", "IDL2", "IDL3", "IDL4", "IDL5", "IDL6", "IDL7" };

/* ---- PIT channel 2 one-shot, polled (interrupts off, before the timebase) ---- */
static int pit2(unsigned int us)
{
	unsigned int cnt = (us * 1193u) / 1000u, v, i;
	if (!cnt) cnt = 1;
	if (cnt > 0xFFFFu) cnt = 0xFFFFu;
	v = inb(0x61) & 0xFCu;
	outb(0x61, v);
	outb(0x43, 0xB0);
	outb(0x42, cnt & 0xFFu);
	outb(0x42, (cnt >> 8) & 0xFFu);
	outb(0x61, v | 1u);
	for (i = 0; i < 4000000u; i++)
		if (inb(0x61) & 0x20u)
			return 0;
	return -1;
}

static unsigned int lr(unsigned int o) { return smp_mmio_rd(lapic + o); }
static void lw(unsigned int o, unsigned int v) { smp_mmio_wr(lapic + o, v); }

static int icr(unsigned int dest, unsigned int lo)
{
	unsigned int i;
	for (i = 0; i < 1000000u && (lr(0x300u) & 0x1000u); i++) ;
	lw(0x310u, dest << 24);
	lw(0x300u, lo);
	for (i = 0; i < 1000000u; i++)
		if ((lr(0x300u) & 0x1000u) == 0)
			return 0;
	return -1;
}

/* ---- firmware tables (MADT, then MP) - as smp.c ---- */
static unsigned int u16(const unsigned char *p) { return p[0] | (p[1] << 8); }
static unsigned int u32(const unsigned char *p) { return p[0] | (p[1]<<8) | (p[2]<<16) | ((unsigned int)p[3]<<24); }
static unsigned int sum(const unsigned char *p, unsigned int n) { unsigned int s=0; while (n--) s += *p++; return s & 0xFFu; }
static int sig(const unsigned char *p, const char *s) { while (*s) if (*p++ != (unsigned char)*s++) return 0; return 1; }

static const unsigned char *scan(unsigned int lo, unsigned int hi, const char *s, unsigned int len)
{
	unsigned int a;
	for (a = lo; a + len <= hi; a += 16u)
		if (sig((const unsigned char *) a, s) && sum((const unsigned char *) a, len) == 0)
			return (const unsigned char *) a;
	return 0;
}

static unsigned int cpu_ids[16];
static void cpu_ids_push(unsigned int id)
{
	int i;
	for (i = 0; i < ncpu; i++)
		if (cpu_ids[i] == id)
			return;                                     /* duplicate enabled entry: count once */
	if (ncpu < 16) cpu_ids[ncpu++] = id;
}

static int detect_cpus(void)
{
	const unsigned char *r, *t, *h, *e, *end; unsigned int rsdt, len, n, i, l, ebda;
	ebda = u16((const unsigned char *) 0x40Eu) << 4;
	r = 0;
	if (ebda >= 0x80000u && ebda < 0xA0000u) r = scan(ebda, ebda + 1024u, "RSD PTR ", 20u);
	if (!r) r = scan(0xE0000u, 0x100000u, "RSD PTR ", 20u);
	if (r) {
		rsdt = u32(r + 16); t = (const unsigned char *) rsdt;
		if (rsdt >= 0x100000u && sig(t, "RSDT")) {
			len = u32(t + 4);
			if (len >= 36u && len < 0x10000u && sum(t, len) == 0) {
				n = (len - 36u) / 4u;
				for (i = 0; i < n; i++) {
					h = (const unsigned char *) u32(t + 36 + 4 * i);
					if ((unsigned int) h < 0x100000u || !sig(h, "APIC")) continue;
					l = u32(h + 4);
					if (l < 44u || l > 0x10000u || sum(h, l) != 0) break;
					end = h + l;
					for (e = h + 44; e + 2 <= end && e[1] >= 2 && e + e[1] <= end; e += e[1])
						if (e[0] == 0 && e[1] >= 8 && (u32(e + 4) & 1u)) cpu_ids_push(e[3]);
					return 1;
				}
			}
		}
	}
	{
		const unsigned char *f = 0, *c; unsigned int base = u16((const unsigned char *) 0x413u) * 1024u, cnt;
		if (ebda >= 0x80000u && ebda < 0xA0000u) f = scan(ebda, ebda + 1024u, "_MP_", 16u);
		if (!f && base >= 0x80000u && base <= 0xA0000u) f = scan(base - 1024u, base, "_MP_", 16u);
		if (!f) f = scan(0xF0000u, 0x100000u, "_MP_", 16u);
		if (!f || f[8] != 1 || f[11] != 0) return 0;
		c = (const unsigned char *) u32(f + 4);
		if (!c || !sig(c, "PCMP")) return 0;
		len = u16(c + 4);
		if (len < 44u || sum(c, len) != 0) return 0;
		cnt = u16(c + 34); e = c + 44; end = c + len;
		for (i = 0; i < cnt && e < end; i++) {
			if (e[0] == 0) { if (e + 20 > end) return 0; if (e[3] & 1u) cpu_ids_push(e[1]); e += 20; }
			else if (e[0] >= 1 && e[0] <= 4) e += 8; else return 0;
		}
		return 2;
	}
}

/* ---- topology: run on the CPU being described (CPUID answers for the executing CPU only) ---- */
static unsigned int bits_for(unsigned int n)     /* smallest b with (1 << b) >= n */
{
	unsigned int b = 0;
	while ((1u << b) < n && b < 8u) b++;
	return b;
}

static void topo_probe(struct cpu_ent *c)
{
	unsigned int r[4], max, logical, cores;

	c->method = TOPO_UNKNOWN;
	c->smt_shift = c->core_shift = 0;
	c->cpuid_apic = c->apic;
	if (!smp_has_cpuid())
		return;
	smp_cpuid(0, r);
	max = r[0];
	smp_cpuid(1, r);
	c->cpuid_apic = r[1] >> 24;
	if (max >= 0xBu) {
		unsigned int l0[4], l1[4];
		smp_cpuid2(0xBu, 0, l0);
		smp_cpuid2(0xBu, 1, l1);
		if ((l0[1] & 0xFFFFu) != 0 && ((l0[2] >> 8) & 0xFFu) == 1u && ((l1[2] >> 8) & 0xFFu) == 2u) {
			c->smt_shift = l0[0] & 0x1Fu;
			c->core_shift = l1[0] & 0x1Fu;
			c->cpuid_apic = l0[3] & 0xFFu;      /* x2APIC id; the 8-bit LAPIC id must match below */
			c->method = TOPO_LEAF0B;
			return;
		}
	}
	if ((r[3] & (1u << 28)) && max >= 4u) {     /* HTT and leaf 4: legacy Intel method */
		unsigned int r4[4];
		logical = (r[1] >> 16) & 0xFFu;
		smp_cpuid2(4u, 0, r4);
		cores = ((r4[0] >> 26) & 0x3Fu) + 1u;
		if (logical >= 1u && cores >= 1u && logical >= cores && (r4[0] & 0x1Fu) != 0) {
			c->smt_shift = bits_for(logical / cores);
			c->core_shift = bits_for(logical);
			c->method = TOPO_LEGACY;
		}
	}
	/* HTT without leaf 4 (or no HTT): the split between cores and threads cannot be proven: TOPO_UNKNOWN */
}

static void topo_decode(struct cpu_ent *c)
{
	c->thread = (int) (c->apic & ((1u << c->smt_shift) - 1u));
	c->core = (int) ((c->apic & ((1u << c->core_shift) - 1u)) >> c->smt_shift);
	c->pkg = (int) (c->apic >> c->core_shift);
}

/* after all APs reported: validate across CPUs and against the MADT list, then decode or fall back to flat */
static void topo_finish(void)
{
	int i, k, in_madt;
	unsigned int m = cpus[0].method;

	topo = (int) m;
	for (i = 0; i < NCPU; i++) {
		if (!cpus[i].online)
			continue;
		in_madt = 0;
		for (k = 0; k < ncpu; k++)
			if (cpu_ids[k] == cpus[i].apic)
				in_madt = 1;
		if (cpus[i].method != m || cpus[i].smt_shift != cpus[0].smt_shift || cpus[i].core_shift != cpus[0].core_shift
		    || cpus[i].cpuid_apic != cpus[i].apic || !in_madt)
			topo = (m == TOPO_UNKNOWN) ? TOPO_UNKNOWN : TOPO_BAD;
	}
	topo_smt = 0;
	for (i = 0; i < NCPU; i++) {
		if (!cpus[i].online)
			continue;
		if (topo == TOPO_LEAF0B || topo == TOPO_LEGACY)
			topo_decode(&cpus[i]);
		else {
			cpus[i].pkg = 0;
			cpus[i].core = i;                   /* flat: every CPU its own core, no sibling invented */
			cpus[i].thread = 0;
		}
	}
	if (topo == TOPO_LEAF0B || topo == TOPO_LEGACY)
		for (i = 0; i < NCPU; i++)
			for (k = i + 1; k < NCPU; k++)
				if (cpus[i].online && cpus[k].online && cpus[i].pkg == cpus[k].pkg && cpus[i].core == cpus[k].core)
					topo_smt = 1;
	ht_mode = topo_smt ? SMP_HT_AUTO : SMP_HT_ALL;     /* default: AUTO when siblings are proven, else flat ALL */
}

/* ---- pre-paging detection (shellboot, before pg_init) ---- */
void smp_boot(void)
{
	unsigned int r[4], hi, lo; int m;
	cpus[0].online = 1;
	cpus[0].idle_slot = -1;
	if (!smp_has_cpuid()) { reason = 1; return; }
	smp_cpuid(1, r);
	if ((r[3] & (1u << 9)) == 0 || (r[3] & (1u << 5)) == 0) { reason = 2; return; }
	lo = smp_rdmsr(0x1Bu, &hi);
	if ((lo & (1u << 11)) == 0 || hi != 0) { reason = 3; return; }
	lapic = lo & 0xFFFFF000u;
	m = detect_cpus();
	if (!m) { reason = 4; return; }
	bsp_id = lr(LAPIC_ID) >> 24;
	cpus[0].apic = bsp_id;
	cpus[0].online = 1;
	cpus[0].idle_slot = -1;
	topo_probe(&cpus[0]);
	con_puts(m == 1 ? "SMP: ACPI MADT: " : "SMP: MP table: ");
	con_putdec((unsigned long) ncpu);
	con_puts(" CPU(s), BSP ");
	con_putdec(bsp_id);
	con_puts("\n");
}

static unsigned int lapic_calibrate(void)
{
	lw(LAPIC_TDCR, 0x3u);
	lw(LAPIC_TICR, 0xFFFFFFFFu);
	if (pit2(10000u) != 0) return 0;
	return 0xFFFFFFFFu - lr(LAPIC_TCCR);
}

static const char *topo_name(void)
{
	return topo == TOPO_LEAF0B ? "CPUID leaf 0xB" : topo == TOPO_LEGACY ? "CPUID leaf 1+4" :
	       topo == TOPO_BAD ? "inconsistent - flat" : "unknown - flat";
}

/* start one AP as CPU index n; returns 1 when it reported in, 0 otherwise (cleaned up) */
static int start_one(unsigned int id, int n)
{
	unsigned int i, stk; int idle; unsigned char *d; const unsigned char *s;

	unsigned int fl;

	/* earlier APs are already scheduling: the job table changes only under the BKL (held with interrupts off) */
	fl = irq_save();
	bkl_enter();
	idle = job_alloc(idle_names[n], smp_ap_idle_entry, 1);
	if (idle >= 0) {
		idle &= 0xffff;
		jobs[idle].aff = (signed char) n;
	}
	bkl_leave();
	irq_restore(fl);
	if (idle < 0)
		return 0;
	stk = r17_karea_alloc(AP_STACK);
	if (stk == 0) {
		fl = irq_save(); bkl_enter();
		job_cancel(jobs[idle].id);
		bkl_leave(); irq_restore(fl);
		return 0;
	}
	cpus[n].apic = id;
	cpus[n].idle_slot = idle;
	cpus[n].stack = stk;
	g_jobidle[n] = idle;
	smp_ap_tr = kx_install_ap_tss_cpu(n, stk + AP_STACK, pg_read_cr3());
	d = (unsigned char *) TRAMP_PA; s = smp_trampoline;
	while (s < smp_trampoline_end) *d++ = *s++;
	smp_ap_pdbr = pg_read_cr3();
	smp_ap_gdtr = kx_gdtr_addr();
	smp_ap_idtr = kx_idtr_addr();
	smp_ap_esp  = (stk + AP_STACK) & ~15u;
	smp_ap_entered = 0;
	ap_ready = 0;
	if (icr(id, 0x00004500u) == 0) {                     /* INIT */
		pit2(10000u);
		{
			icr(id, 0x00004600u | (TRAMP_PA >> 12));     /* SIPI */
			pit2(200u);
			if (!smp_ap_entered) { icr(id, 0x00004600u | (TRAMP_PA >> 12)); pit2(200u); }
		}
		for (i = 0; i < 200u && !ap_ready; i++) pit2(1000u);   /* barrier: the AP finished smp_ap_main's set-up */
	}
	if (!ap_ready) {
		icr(id, 0x00004500u);                            /* hold a late AP in wait-for-SIPI: it never runs here */
		pit2(10000u);
		fl = irq_save(); bkl_enter();
		job_cancel(jobs[idle].id);
		g_jobidle[n] = g_jobidle[0];
		bkl_leave(); irq_restore(fl);
		cpus[n].online = 0;
		cpus[n].idle_slot = -1;
		if (nfail < 16) fail_ids[nfail++] = id;
		return 0;
	}
	return 1;
}

void smp_start_ap(void)
{
	unsigned int calib; int k, n;

	if (reason != 0 || ncpu < 2) { state = SMP_S_DOWN; topo_finish(); return; }
	pg_map_lapic(lapic);
	calib = lapic_calibrate();
	if (calib == 0) { state = SMP_S_FAILED; reason = 9; return; }
	lvt_count = calib;
	kx_set_ap_gate(AP_TIMER_VEC, (unsigned int) smp_ap_timer_stub);
	cpus[0].idle_slot = g_jobidle[0];
	n = 1;
	for (k = 0; k < ncpu && n < NCPU; k++) {
		if (cpu_ids[k] == bsp_id)
			continue;
		if (start_one(cpu_ids[k], n))
			n++;
	}
	nup = n;
	state = nup > 1 ? SMP_S_UP : SMP_S_FAILED;
	if (nup == 1) reason = 7;
	topo_finish();
	con_puts("SMP: ");
	con_putdec((unsigned long) nup);
	con_puts(" of ");
	con_putdec((unsigned long) ncpu);
	con_puts(" CPU(s) online");
	if (nfail) {
		con_puts(", ");
		con_putdec((unsigned long) nfail);
		con_puts(" did not start (never scheduled)");
	}
	if (ncpu > NCPU) {
		con_puts(", ");
		con_putdec((unsigned long) (ncpu - NCPU));
		con_puts(" beyond the 8-CPU limit ignored");
	}
	con_puts("; topology ");
	con_puts(topo_name());
	con_puts(topo_smt ? ", SMT siblings, HT mode AUTO" : ", no SMT siblings proven, HT mode ALL");
	con_puts("; CPU 0 keeps the devices\n");
}

/* ---- the APs ---- */
void smp_ap_idle_entry(void) { for (;;) __asm__("hlt"); }   /* R17: halts between timer ticks (no host spin) */

void smp_ap_main(void)
{
	extern void fpu_reset(void);
	int c = kx_this_cpu();
	fpu_reset();
	lw(LAPIC_SVR, 0x1FFu);
	cpus[c].apic = lr(LAPIC_ID) >> 24;
	topo_probe(&cpus[c]);
	lw(LAPIC_TDCR, 0x3u);
	lw(LAPIC_LVTT, AP_TIMER_VEC | 0x20000u);
	lw(LAPIC_TICR, lvt_count);
	g_jobcur[c] = g_jobidle[c];
	jobs[g_jobidle[c]].running_on = (signed char) c;
	cpus[c].online = 1;
	ap_ready = 1;                                        /* the barrier CPU 0 waits on */
	__asm__("sti");
	for (;;)
		__asm__("hlt");
}

unsigned int smp_ap_timer_resume(unsigned int sp)
{
	int c = kx_this_cpu();
	cpus[c].ticks++;
	lw(LAPIC_EOI, 0);
	smp_acct_tick(sp);
	cpus[c].dispatch++;
	return schedule_entry(sp);
}

/* ---- accounting (R16B semantics, N CPUs; all under the BKL) ---- */
extern volatile unsigned long sched_ticks;
extern unsigned long bkl_wait_kc[8], bkl_contended[8];
int vcon_fg(void);
int vcon_job_con(int slot);
static unsigned long st_ticks[NCPU], st_cpl3[NCPU], st_cpl0[NCPU], st_idle[NCPU], st_ctxsw[NCPU], st_migr[NCPU];
static unsigned long st_req = 0, st_polmoves = 0, st_unpinned = 0, st_rebal = 0;
#ifndef R16B_POLICY_DEFAULT
#define R16B_POLICY_DEFAULT SMP_POL_FG
#endif
static unsigned int policy = R16B_POLICY_DEFAULT;
volatile unsigned long smp_focus_gen = 0;

void smp_acct_tick(unsigned int sp)
{
	int c = kx_this_cpu();
	unsigned int *f = (unsigned int *) sp;

	st_ticks[c]++;
	if (g_jobcur[c] == g_jobidle[c])
		st_idle[c]++;
	else if ((f[9] & 3u) == 3u)
		st_cpl3[c]++;
	else
		st_cpl0[c]++;
}

void smp_note_dispatch(int slot, int cpu)
{
	st_ctxsw[cpu]++;
	if (cpu != 0)
		jobs[slot].lent_tick = 0;
	if (jobs[slot].last_cpu >= 0 && jobs[slot].last_cpu != cpu)
		st_migr[cpu]++;
}

/* ---- placement ---- */
static int sibling_of_cpu0(int c)
{
	return topo_smt && c != 0 && cpus[c].pkg == cpus[0].pkg && cpus[c].core == cpus[0].core;
}

/* may background work run on CPU c under the current HT mode?  (CPU 0 is always allowed: it is the foreground CPU) */
static int cpu_allowed(int c)
{
	if (c < 0 || c >= NCPU || !cpus[c].online)
		return 0;
	if (c == 0)
		return 1;
	if (ht_mode == SMP_HT_PHYSICAL && topo_smt && cpus[c].thread != 0)
		return 0;                                        /* an HT sibling (CPU 0's included): parked */
	return 1;
}

int r17_cpu_parked(int c)                                /* pick_next: a parked CPU runs only its idle job */
{
	return c > 0 && c < NCPU && cpus[c].online && !cpu_allowed(c);
}

/* preference rank of a background CPU: AUTO puts distinct cores first (thread 0 of cores other than CPU 0's), then the
 * other siblings, CPU 0's sibling last; ALL is plain index order. */
static int rank(int c)
{
	if (ht_mode == SMP_HT_ALL || !topo_smt)
		return c;
	if (sibling_of_cpu0(c))
		return 3 * NCPU + c;
	return (cpus[c].thread == 0 ? 0 : NCPU) + c;
}

static int load_of(int c, int except)
{
	int s, n = 0;
	for (s = 1; s < JOB_MAX; s++)
		if (s != except && jobs[s].in_use && !jobs[s].dead && jobs[s].aff == (signed char) c && pg_slot_prot(s))
			n++;
	return n;
}

/* the least-loaded allowed background CPU (lexicographic: load, then rank); AUTO counts CPU 0's sibling one heavier */
static int pick_bg(int except)
{
	int c, best = -1, bl = 0, br = 0, l, r;
	for (c = 1; c < NCPU; c++) {
		if (!cpu_allowed(c))
			continue;
		l = load_of(c, except) + ((ht_mode == SMP_HT_AUTO && sibling_of_cpu0(c)) ? 1 : 0);
		r = rank(c);
		if (best < 0 || l < bl || (l == bl && r < br)) {
			best = c; bl = l; br = r;
		}
	}
	return best < 0 ? 0 : best;
}

static int r16b_effective(int slot)
{
	if (jobs[slot].pin >= 0)
		return jobs[slot].pin;
	if (state != SMP_S_UP || policy == SMP_POL_OFF)
		return 0;
	if (vcon_job_con(slot) == vcon_fg())
		return 0;
	/* background: stay where it is if that CPU is still an allowed background CPU, else the least-loaded one */
	if (jobs[slot].aff > 0 && cpu_allowed(jobs[slot].aff))
		return jobs[slot].aff;
	return pick_bg(slot);
}

static int r16b_set_aff(int slot, int cpu)
{
	if (jobs[slot].aff == (signed char) cpu)
		return 0;
	jobs[slot].aff = (signed char) cpu;
	jobs[slot].moved_tick = sched_ticks;
	jobs[slot].lent_tick = 0;
	st_req++;
	return 1;
}

#ifndef R16B_POL_SWEEP
#define R16B_POL_SWEEP     50u
#endif
#ifndef R16B_POL_RESIDENCY
#define R16B_POL_RESIDENCY 25u
#endif
static unsigned long pol_gen = 0, pol_last = 0;
static int pol_deferred = 0;
unsigned long st_poldefer = 0;

void smp_policy_tick(void)
{
	int s, want, c, hi = -1, lo = -1, lh = 0, ll = 0, l;

	if (policy != SMP_POL_FG || state != SMP_S_UP)
		return;
	if (smp_focus_gen == pol_gen && !pol_deferred && sched_ticks - pol_last < R16B_POL_SWEEP)
		return;
	if (smp_focus_gen == pol_gen && pol_deferred && sched_ticks - pol_last < R16B_POL_RESIDENCY / 2u)
		return;
	pol_gen = smp_focus_gen;
	pol_last = sched_ticks;
	pol_deferred = 0;
	for (s = 1; s < JOB_MAX; s++) {
		if (!jobs[s].in_use || jobs[s].dead || jobs[s].pin >= 0 || !pg_slot_prot(s))
			continue;
		want = r16b_effective(s);
		if (jobs[s].aff == (signed char) want)
			continue;
		if (sched_ticks - jobs[s].moved_tick < R16B_POL_RESIDENCY && cpu_allowed(jobs[s].aff)) {
			pol_deferred = 1;                            /* too soon (a CPU being parked is left at once) */
			st_poldefer++;
			continue;
		}
		if (r16b_set_aff(s, want))
			st_polmoves++;
	}
	/* rebalance: when two allowed background CPUs differ by two or more AUTO jobs, move one (respecting residency) */
	for (c = 1; c < NCPU; c++) {
		if (!cpu_allowed(c))
			continue;
		l = load_of(c, -1) + ((ht_mode == SMP_HT_AUTO && sibling_of_cpu0(c)) ? 1 : 0);
		if (hi < 0 || l > lh) { hi = c; lh = l; }
		if (lo < 0 || l < ll || (l == ll && rank(c) < rank(lo))) { lo = c; ll = l; }
	}
	if (hi > 0 && lo > 0 && lh - ll >= 2)
		for (s = 1; s < JOB_MAX; s++)
			if (jobs[s].in_use && !jobs[s].dead && jobs[s].pin < 0 && pg_slot_prot(s) && jobs[s].aff == (signed char) hi
			    && sched_ticks - jobs[s].moved_tick >= R16B_POL_RESIDENCY) {
				r16b_set_aff(s, lo);
				st_rebal++;
				break;
			}
}

/* R17: the R16B LEND policy is not offered (its borrowing rule is defined for two CPUs) */
int smp_steal(int cpu)
{
	(void) cpu;
	return -1;
}

/* HT mode change: re-place every AUTO job at the next policy pass; a manual pin on a CPU that becomes parked is
 * cleared (the job returns to AUTO) so nothing is left stranded on a parked CPU. */
static int set_ht(unsigned int m)
{
	int s;
	if (m != SMP_HT_ALL && m != SMP_HT_AUTO && m != SMP_HT_PHYSICAL)
		return SMP_ERR_ARG;
	if (m != SMP_HT_ALL && !topo_smt)
		return SMP_ERR_NOTOPO;
	ht_mode = m;
	for (s = 1; s < JOB_MAX; s++)
		if (jobs[s].in_use && jobs[s].pin > 0 && !cpu_allowed(jobs[s].pin)) {
			jobs[s].pin = -1;
			st_unpinned++;
		}
	for (s = 1; s < JOB_MAX; s++)                        /* AUTO jobs on a now-parked CPU: move now, not after residency */
		if (jobs[s].in_use && !jobs[s].dead && jobs[s].pin < 0 && pg_slot_prot(s) && jobs[s].aff > 0 && !cpu_allowed(jobs[s].aff))
			r16b_set_aff(s, policy == SMP_POL_FG ? pick_bg(s) : 0);
	smp_focus_gen++;
	return 0;
}

static unsigned int cpu_flags(int c)
{
	unsigned int f = 0;
	if (cpus[c].online) f |= 1u;
	if (r17_cpu_parked(c)) {
		f |= 2u;
		if (g_jobcur[c] != g_jobidle[c]) f |= 4u;          /* parking: still finishing the job it had */
	}
	if (sibling_of_cpu0(c)) f |= 8u;
	return f;
}

static int r17_stats(unsigned int words, unsigned int buf)
{
	unsigned int w[SMP_STATS_WORDS2], i, n;
	int c;

	if (words < 16u)
		return SMP_ERR_ARG;
	n = words < SMP_STATS_WORDS2 ? words : SMP_STATS_WORDS2;
	if (!chan_user_range_ok(buf, n * 4u))
		return CHAN_ERR_RANGE;
	for (i = 0; i < SMP_STATS_WORDS2; i++)
		w[i] = 0;
	w[0] = SMP_STATS_VERSION2;
	w[1] = (unsigned int) smp_cpu_count();
	w[2] = (unsigned int) state;
	w[3] = policy;
	w[4] = (unsigned int) smp_focus_gen;
	w[5] = (unsigned int) vcon_fg();
	w[6] = (unsigned int) st_req;
	w[7] = (unsigned int) st_polmoves;
	w[8] = (unsigned int) sched_ticks;
	w[9] = (unsigned int) st_poldefer;
	w[10] = (unsigned int) st_rebal;
	w[11] = bkl_word;
	w[12] = (unsigned int) bkl_badrel;
	w[13] = (unsigned int) bkl_maxspin;
	w[14] = ht_mode;
	w[15] = (unsigned int) topo | ((unsigned int) topo_smt << 8) | ((unsigned int) ncpu << 16) | (st_unpinned << 24);
	for (c = 0; c < NCPU; c++) {
		unsigned int *p = w + 16 + 16 * c;
		p[0] = (unsigned int) st_ticks[c];
		p[1] = (unsigned int) st_cpl3[c];
		p[2] = (unsigned int) st_cpl0[c];
		p[3] = (unsigned int) st_idle[c];
		p[4] = (unsigned int) st_ctxsw[c];
		p[5] = (unsigned int) st_migr[c];
		p[6] = (unsigned int) bkl_acq[c];
		p[7] = (unsigned int) bkl_contended[c];
		p[8] = (unsigned int) bkl_spins[c];
		p[9] = (unsigned int) bkl_wait_kc[c];
		p[10] = cpus[c].online ? jobs[g_jobcur[c]].id : 0u;
		p[11] = cpus[c].online ? jobs[g_jobidle[c]].id : 0u;
		p[12] = (unsigned int) bkl_depth[c];
		p[13] = cpus[c].apic;
		p[14] = ((unsigned int) cpus[c].pkg << 16) | (((unsigned int) cpus[c].core & 0xFFu) << 8) | ((unsigned int) cpus[c].thread & 0xFFu);
		p[15] = cpu_flags(c) | ((unsigned int) load_of(c, -1) << 8);
	}
	{ unsigned int *u = (unsigned int *) buf; for (i = 0; i < n; i++) u[i] = w[i]; }
	return (int) n;
}

/* SMP_OP_TOPO: w0 version 1, w1 method, w2 smt proven, w3 MADT count, w4 online, w5 ht_mode, w6 failed starts,
 * w7 BSP id; w8..23 MADT APIC ids; w24..39 failed APIC ids; per CPU c at 40 + 7c: apic, cpuid_apic, method,
 * smt_shift | core_shift << 8, pkg, core, thread | flags << 8. */
static int r17_topo(unsigned int words, unsigned int buf)
{
	unsigned int w[SMP_TOPO_WORDS], i, n;
	int c;

	n = words < SMP_TOPO_WORDS ? words : SMP_TOPO_WORDS;
	if (n < 8u)
		return SMP_ERR_ARG;
	if (!chan_user_range_ok(buf, n * 4u))
		return CHAN_ERR_RANGE;
	for (i = 0; i < SMP_TOPO_WORDS; i++)
		w[i] = 0;
	w[0] = 1u; w[1] = (unsigned int) topo; w[2] = (unsigned int) topo_smt; w[3] = (unsigned int) ncpu;
	w[4] = (unsigned int) nup; w[5] = ht_mode; w[6] = nfail; w[7] = bsp_id;
	for (i = 0; i < 16u; i++) {
		w[8 + i] = (int) i < ncpu ? cpu_ids[i] : 0xFFFFFFFFu;
		w[24 + i] = i < nfail ? fail_ids[i] : 0xFFFFFFFFu;
	}
	for (c = 0; c < NCPU; c++) {
		unsigned int *p = w + 40 + 7 * c;
		p[0] = cpus[c].online ? cpus[c].apic : 0xFFFFFFFFu;
		p[1] = cpus[c].cpuid_apic;
		p[2] = cpus[c].method;
		p[3] = cpus[c].smt_shift | (cpus[c].core_shift << 8);
		p[4] = (unsigned int) cpus[c].pkg;
		p[5] = (unsigned int) cpus[c].core;
		p[6] = ((unsigned int) cpus[c].thread & 0xFFu) | (cpu_flags(c) << 8);
	}
	{ unsigned int *u = (unsigned int *) buf; for (i = 0; i < n; i++) u[i] = w[i]; }
	return (int) n;
}

/* ---- control trap ---- */
int smp_cpu_count(void) { return nup; }
int smp_affinity(int slot) { return (slot >= 0 && slot < JOB_MAX) ? jobs[slot].aff : 0; }

int smp_trap(unsigned int a, unsigned int b, unsigned int c)
{
	unsigned int w[SMP_INFO_WORDS], i; int slot;
	switch (a) {
	case SMP_OP_INFO:
		if (!chan_user_range_ok(c, sizeof w)) return CHAN_ERR_RANGE;
		for (i = 0; i < SMP_INFO_WORDS; i++) w[i] = 0;
		w[0] = (unsigned int) state; w[1] = (unsigned int) reason; w[2] = (unsigned int) ncpu;
		w[3] = bsp_id; w[4] = cpus[1].apic; w[5] = lapic; w[6] = lvt_count;
		w[7] = (unsigned int) cpus[1].ticks; w[8] = (unsigned int) cpus[1].dispatch;
		w[9] = bkl_word; w[10] = (unsigned int) bkl_acq[0]; w[11] = (unsigned int) bkl_acq[1];
		w[12] = (unsigned int) bkl_rel[0]; w[13] = (unsigned int) bkl_rel[1];
		w[14] = (unsigned int) bkl_spins[0]; w[15] = (unsigned int) bkl_spins[1];
		w[16] = (unsigned int) bkl_maxspin; w[17] = (unsigned int) bkl_badrel;
		w[18] = (unsigned int) bkl_depth[0]; w[19] = (unsigned int) bkl_depth[1];
		w[20] = (unsigned int) jobs[g_jobcur[0]].id; w[21] = (unsigned int) (cpus[1].online ? jobs[g_jobcur[1]].id : 0);
		w[22] = (unsigned int) g_jobidle[1]; w[23] = (unsigned int) kx_this_cpu();
		{ unsigned int *u = (unsigned int *) c; for (i = 0; i < SMP_INFO_WORDS; i++) u[i] = w[i]; }
		return 0;
	case SMP_OP_SETAFF:
		/* R17: pin a live CPL3 job to CPU 0..7 (online, not parked), or SMP_CPU_AUTO8.  A request only: the owning CPU
		 * honours it at its next schedule() pass. */
		slot = job_lookup(b);
		if (slot == JOB_NONE)
			return SMP_ERR_NOJOB;
		if (c > SMP_CPU_AUTO8)
			return SMP_ERR_ARG;
		if (!pg_slot_prot(slot))
			return SMP_ERR_RING0;
		if (jobs[slot].dead)
			return SMP_ERR_DEAD;
		if (c != SMP_CPU_AUTO8 && (c >= (unsigned int) NCPU || !cpus[c].online))
			return SMP_ERR_UNAVAIL;
		if (c != SMP_CPU_AUTO8 && !cpu_allowed((int) c))
			return SMP_ERR_PARKED;
		jobs[slot].pin = (signed char) (c == SMP_CPU_AUTO8 ? -1 : (int) c);
		(void) r16b_set_aff(slot, r16b_effective(slot));
		return 0;
	case SMP_OP_STATS:
		return r17_stats(b, c);
	case SMP_OP_POLICY:
		{
			unsigned int prev = policy;
			if (b == SMP_POL_QUERY)
				return (int) prev;
			if (b != SMP_POL_OFF && b != SMP_POL_FG)
				return SMP_ERR_ARG;                  /* LEND is not offered by R17 */
			policy = b;
			smp_focus_gen++;
			return (int) prev;
		}
	case SMP_OP_MEMINFO:
		return proc_meminfo(b, c);
	case SMP_OP_TOPO:
		return r17_topo(b, c);
	case SMP_OP_HT:
		{
			unsigned int prev = ht_mode;
			int rc;
			if (b == SMP_POL_QUERY)
				return (int) prev;
			rc = set_ht(b);
			return rc < 0 ? rc : (int) prev;
		}
	case SMP_OP_START:
		return state;
	}
	return SMP_ERR_ARG;
}
