/*
 * QDOS/386 R16 - two-CPU scheduling (CPU 0 master, CPU 1 worker), docs/R16-ARCHITECTURE.md.
 * CPU 0 owns every device and the timebase.  CPU 1 runs CPL3-affine jobs and enters the shared kernel only under the
 * BKL (kernel/bkl.c).  Discovery and INIT/SIPI reuse the method proven in kernel/amp.c, but the AP here is a full
 * paging-enabled kernel CPU with its own TSS, stack and LAPIC timer - it is NOT the paging-off AMP0 worker.
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

/* kctx3.s / smp_s.s */
int smp_has_cpuid(void);
void smp_cpuid(unsigned int leaf, unsigned int *r);
unsigned int smp_rdmsr(unsigned int msr, unsigned int *hi);
unsigned int smp_mmio_rd(unsigned int a);
void smp_mmio_wr(unsigned int a, unsigned int v);
extern unsigned char smp_trampoline[], smp_trampoline_end[];
extern unsigned int smp_ap_pdbr, smp_ap_gdtr, smp_ap_idtr, smp_ap_esp, smp_ap_entered;
void smp_ap_timer_stub(void);
void smp_ap_idle_entry(void);
void smp_ap_main(void);
unsigned int smp_ap_timer_resume(unsigned int sp);
int kx_this_cpu(void);
unsigned int schedule_entry(unsigned int sp);   /* jobs.c: a scheduler pass for an AP timer (no timekeeping) */

/* descriptors.c */
extern unsigned int kx_gdtr_addr(void);
extern unsigned int kx_idtr_addr(void);
void kx_install_ap_tss(unsigned int esp0, unsigned int cr3);
void kx_set_ap_gate(int vector, unsigned int handler);
void kx_set_esp0_cpu(int cpu, unsigned int esp);

#define TRAMP_PA     0x00009000u
#define LAPIC_ID     0x020u
#define LAPIC_SVR    0x0F0u
#define LAPIC_EOI    0x0B0u
#define LAPIC_LVTT   0x320u
#define LAPIC_TDCR   0x3E0u
#define LAPIC_TICR   0x380u
#define LAPIC_TCCR   0x390u
#define AP_TIMER_VEC 0x40u

static int state = SMP_S_DOWN, reason = 0, ncpu = 0;
static unsigned int lapic = 0, bsp_id = 0, ap_id = 0, lvt_count = 0;
static unsigned char ap_stack[16384];
static unsigned long ap_ticks = 0, ap_dispatch = 0;

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

/* ---- LAPIC ---- */
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

/* ---- firmware tables (compact MADT, then MP) ---- */
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
static void cpu_ids_push(unsigned int id) { if (ncpu < 16) cpu_ids[ncpu++] = id; }

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
						if (e[0] == 0 && e[1] >= 8 && (u32(e + 4) & 1u) && ncpu < 16) cpu_ids_push(e[3]);
					return 1;
				}
			}
		}
	}
	/* MP table fallback */
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
			if (e[0] == 0) { if (e + 20 > end) return 0; if ((e[3] & 1u) && ncpu < 16) cpu_ids_push(e[1]); e += 20; }
			else if (e[0] >= 1 && e[0] <= 4) e += 8; else return 0;
		}
		return 2;
	}
}
/* ---- pre-paging detection (shellboot, before pg_init) ---- */
void smp_boot(void)
{
	unsigned int r[4], hi, lo; int m, i;
	if (!smp_has_cpuid()) { reason = 1; return; }
	smp_cpuid(1, r);
	if ((r[3] & (1u << 9)) == 0 || (r[3] & (1u << 5)) == 0) { reason = 2; return; }
	lo = smp_rdmsr(0x1Bu, &hi);
	if ((lo & (1u << 11)) == 0 || hi != 0) { reason = 3; return; }
	lapic = lo & 0xFFFFF000u;
	m = detect_cpus();
	if (!m) { reason = 4; return; }
	bsp_id = lr(LAPIC_ID) >> 24;
	for (i = 0; i < ncpu; i++) if (cpu_ids[i] != bsp_id) { ap_id = cpu_ids[i]; break; }
	con_puts(m == 1 ? "SMP: ACPI MADT: " : "SMP: MP table: ");
	con_putdec((unsigned long) ncpu);
	con_puts(" CPU(s), BSP ");
	con_putdec(bsp_id);
	con_puts("\n");
}

static unsigned int lapic_calibrate(void)   /* LAPIC timer ticks in one 10 ms PIT interval, divide-by-16 */
{
	lw(LAPIC_TDCR, 0x3u);                 /* divide by 16 */
	lw(LAPIC_TICR, 0xFFFFFFFFu);
	if (pit2(10000u) != 0) return 0;
	return 0xFFFFFFFFu - lr(LAPIC_TCCR);
}

/* ---- bring up CPU 1 (shellboot, after pg_init + the first process is created, after timer_start) ---- */
void smp_start_ap(void)
{
	unsigned int i, calib; unsigned char *d; const unsigned char *s; int ap_idle;

	if (reason != 0 || ncpu < 2) { state = SMP_S_DOWN; return; }
	pg_map_lapic(lapic);                 /* LAPIC reachable with paging on, in every (template-cloned) directory */

	/* an idle job pinned to CPU 1 */
	ap_idle = job_alloc("IDL1", smp_ap_idle_entry, 1);
	if (ap_idle < 0) { state = SMP_S_FAILED; reason = 5; return; }
	jobs[ap_idle].aff = 1;
	g_jobidle[1] = ap_idle & 0xffff;

	calib = lapic_calibrate();
	if (calib == 0) { state = SMP_S_FAILED; reason = 9; return; }
	lvt_count = calib;

	kx_set_ap_gate(AP_TIMER_VEC, (unsigned int) smp_ap_timer_stub);
	kx_install_ap_tss((unsigned int) (ap_stack + sizeof ap_stack), pg_read_cr3());

	d = (unsigned char *) TRAMP_PA; s = smp_trampoline;
	while (s < smp_trampoline_end) *d++ = *s++;
	smp_ap_pdbr = pg_read_cr3();
	smp_ap_gdtr = kx_gdtr_addr();
	smp_ap_idtr = kx_idtr_addr();
	smp_ap_esp  = (unsigned int) (ap_stack + sizeof ap_stack) & ~15u;
	smp_ap_entered = 0;

	if (icr(ap_id, 0x00004500u) != 0) { state = SMP_S_FAILED; reason = 6; return; }   /* INIT */
	pit2(10000u);
	if (icr(ap_id, 0x00004600u | (TRAMP_PA >> 12)) != 0) { state = SMP_S_FAILED; reason = 6; return; }  /* SIPI */
	pit2(200u);
	if (!smp_ap_entered) { icr(ap_id, 0x00004600u | (TRAMP_PA >> 12)); pit2(200u); }
	for (i = 0; i < 200u && !smp_ap_entered; i++) pit2(1000u);
	if (!smp_ap_entered) { state = SMP_S_FAILED; reason = 7; return; }
	state = SMP_S_UP;
	con_puts("SMP: CPU 1 (LAPIC id ");
	con_putdec(ap_id);
	con_puts(") up, paging on, own TSS/stack/timer; CPU 0 keeps the devices\n");
}

/* ---- the AP ---- */
void smp_ap_idle_entry(void) { for (;;) ; }   /* runs at ring 0 on CPU 1 between jobs; preempted by the LAPIC timer */

void smp_ap_main(void)
{
	/* runs on CPU 1, paging on, BKL not held.  Program the LAPIC timer (periodic) and enable interrupts; the first
	 * tick enters smp_ap_timer_stub and the scheduler takes over. */
	extern void fpu_reset(void);
	fpu_reset();
	lw(LAPIC_SVR, 0x1FFu);                 /* software-enable the LAPIC, spurious vector 0xFF */
	lw(LAPIC_TDCR, 0x3u);
#ifdef QDOS_R16_APNOTIMER
	lw(LAPIC_LVTT, AP_TIMER_VEC | 0x20000u | 0x10000u);   /* DIAG: masked - AP dormant */
#else
	lw(LAPIC_LVTT, AP_TIMER_VEC | 0x20000u);   /* periodic */
#endif
	lw(LAPIC_TICR, lvt_count);
	g_jobcur[1] = g_jobidle[1];
	jobs[g_jobidle[1]].running_on = 1;
	__asm__("sti");
	for (;;)
		__asm__("hlt");
}

/* called by smp_ap_timer_stub (BKL held): LAPIC EOI, a scheduler pass with no timekeeping, return the resume sp */
unsigned int smp_ap_timer_resume(unsigned int sp)
{
	ap_ticks++;
	lw(LAPIC_EOI, 0);
	ap_dispatch++;
	return schedule_entry(sp);
}

/* ---- affinity / control trap (CPU 0) ---- */
int smp_cpu_count(void) { return state == SMP_S_UP ? 2 : 1; }
int smp_affinity(int slot) { return (slot >= 0 && slot < JOB_MAX) ? jobs[slot].aff : 0; }

int smp_trap(unsigned int a, unsigned int b, unsigned int c)
{
	unsigned int w[SMP_INFO_WORDS], i; int slot;
	switch (a) {
	case SMP_OP_INFO:
		if (!chan_user_range_ok(c, sizeof w)) return CHAN_ERR_RANGE;
		for (i = 0; i < SMP_INFO_WORDS; i++) w[i] = 0;
		w[0] = (unsigned int) state; w[1] = (unsigned int) reason; w[2] = (unsigned int) ncpu;
		w[3] = bsp_id; w[4] = ap_id; w[5] = lapic; w[6] = lvt_count;
		w[7] = (unsigned int) ap_ticks; w[8] = (unsigned int) ap_dispatch;
		w[9] = bkl_word; w[10] = (unsigned int) bkl_acq[0]; w[11] = (unsigned int) bkl_acq[1];
		w[12] = (unsigned int) bkl_rel[0]; w[13] = (unsigned int) bkl_rel[1];
		w[14] = (unsigned int) bkl_spins[0]; w[15] = (unsigned int) bkl_spins[1];
		w[16] = (unsigned int) bkl_maxspin; w[17] = (unsigned int) bkl_badrel;
		w[18] = (unsigned int) bkl_depth[0]; w[19] = (unsigned int) bkl_depth[1];
		w[20] = (unsigned int) jobs[g_jobcur[0]].id; w[21] = (unsigned int) (state == SMP_S_UP ? jobs[g_jobcur[1]].id : 0);
		w[22] = (unsigned int) g_jobidle[1]; w[23] = kx_this_cpu();
		{ unsigned int *u = (unsigned int *) c; for (i = 0; i < SMP_INFO_WORDS; i++) u[i] = w[i]; }
		return 0;
	case SMP_OP_SETAFF:
		slot = job_lookup(b);
		if (slot == JOB_NONE || (int) c < 0 || c > 1) return SMP_ERR_ARG;
		if (c == 1 && state != SMP_S_UP) return SMP_ERR_UNAVAIL;
		if (c == 1 && !pg_slot_prot(slot)) return SMP_ERR_RING0;   /* only CPL3 jobs may run on CPU 1 */
		/* A job may re-pin ITSELF (it is "running" on the calling CPU; the move happens at its next reschedule).
		 * Forbid only moving a job that is running on the OTHER CPU - that would need a cross-CPU save. */
		if (jobs[slot].running_on != -1 && slot != g_jobcur[kx_this_cpu()]) return SMP_ERR_STATE;
		jobs[slot].aff = (signed char) c;
		return 0;
	case SMP_OP_START:
		if (state == SMP_S_UP) return SMP_S_UP;
		smp_start_ap();
		return state;
	}
	return SMP_ERR_ARG;
}
