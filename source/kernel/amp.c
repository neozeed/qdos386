/*
 * QDOS/386 R15 AMP0 - CPU 0 (BSP) side of the master/worker service (docs/R15-AMP-ARCHITECTURE.md).  -DQDOS_AMP only.
 *
 * Boot (amp_boot, called by shell_boot BEFORE paging is enabled, so LAPIC MMIO and the ACPI/MP tables are reached at
 * their physical addresses and nothing has to be mapped):
 *   1. capability: CPUID present, CPUID.1:EDX.APIC, IA32_APIC_BASE enabled - otherwise "unavailable", fail closed
 *   2. processor inventory from the ACPI MADT (RSDP in the EBDA / 0xE0000-0xFFFFF, every checksum checked), else from
 *      the Intel MP table (_MP_ / PCMP, checksums checked); the BSP's own LAPIC ID is read, never assumed; the worker is
 *      the first ENABLED processor whose APIC ID differs from the BSP's
 *   3. trampoline copied to 0x8000, worker IDT and stack prepared, mailbox reset; INIT, 10 ms, SIPI (vector 0x08),
 *      200 us, a second SIPI only if the worker has not started; ICR delivery status polled with bounds; delays from PIT
 *      channel 2 (no interrupts needed)
 *   4. wait up to 200 ms for AP_ST_READY, then check magic, ABI version, boot generation and APIC ID
 * A worker that answers late is never adopted and its memory is never reused (it is static BSS in any case).
 *
 * Service (QT_AMP trap, CPU 0 only, IF clear): one request slot; the caller's buffers are checked with
 * chan_user_range_ok and COPIED into / out of the pinned worker buffers - the worker never sees an application
 * pointer.  A slot belongs to the submitting job; a slot whose owner has ended is reclaimed only after the worker has
 * published DONE / ERROR / CANCELLED (or has positively acknowledged a halt) - never on a timeout.
 */
#include "jobs.h"
#include "platform.h"
#include "channel.h"
#include "amp.h"
#ifdef QDOS_PAGING
#include "paging.h"
#endif

extern void outb(unsigned int port, unsigned int val);
extern unsigned int inb(unsigned int port);
extern unsigned int amp_ap_stack[];

int amp_has_cpuid(void);
void amp_cpuid(unsigned int leaf, unsigned int *r);
unsigned int amp_rdmsr(unsigned int msr, unsigned int *hi);
unsigned int amp_load(volatile unsigned int *p);
unsigned int amp_xchg(volatile unsigned int *p, unsigned int v);
unsigned int amp_cas(volatile unsigned int *p, unsigned int old, unsigned int nw);
void amp_pause(void);
unsigned int amp_mmio_rd(unsigned int a);
void amp_mmio_wr(unsigned int a, unsigned int v);

#define SVC_UNAVAIL   0
#define SVC_READY     1
#define SVC_FAULTED   2
#define SVC_QUIESCED  3
#define SVC_STARTING  4

#define LAPIC_ID      0x020u
#define LAPIC_VER     0x030u
#define LAPIC_SVR     0x0F0u
#define LAPIC_ESR     0x280u
#define LAPIC_ICRLO   0x300u
#define LAPIC_ICRHI   0x310u

#define MAXCPU 16

static int svc = SVC_UNAVAIL, reason = AMP_R_NONE, method = 0, ncpu = 0, late_seen = 0;
static unsigned int lapic = 0, bsp_id = 0, ap_id = 0, lapic_ver = 0, lapic_svr = 0, esr_seen = 0;
static unsigned int cpu_ids[MAXCPU];
static unsigned int next_gen = 1, owner = 0, cur_gen = 0, orphans = 0, cancels = 0, boots = 0, ready_us = 0;
static unsigned char amp_pt_mem[8192];

static const char *const rname[] = {
	"", "no CPUID instruction", "CPUID: no local APIC (or no MSR support)", "local APIC disabled (IA32_APIC_BASE)",
	"no valid ACPI MADT or MP table", "only one enabled processor", "ICR delivery status stuck",
	"worker did not report READY within 200 ms", "worker identity check failed", "PIT channel 2 not counting",
	"AMP disabled in this build"
};

/* ---- timing: PIT channel 2 one-shot (mode 0), polled - works with interrupts off and before the timer runs ---- */
static int pit2_wait_us(unsigned int us)
{
	unsigned int cnt = (us * 1193u) / 1000u, v, i;

	if (cnt == 0)
		cnt = 1;
	if (cnt > 0xFFFFu)
		cnt = 0xFFFFu;
	v = inb(0x61) & 0xFCu;                 /* gate off, speaker off */
	outb(0x61, v);
	outb(0x43, 0xB0);                      /* channel 2, lobyte/hibyte, mode 0, binary */
	outb(0x42, cnt & 0xFFu);
	outb(0x42, (cnt >> 8) & 0xFFu);
	outb(0x61, v | 0x01u);                 /* gate on: count */
	for (i = 0; i < 4000000u; i++)
		if (inb(0x61) & 0x20u)
			return 0;
	return -1;
}

static int wait_ms(unsigned int ms)
{
	while (ms-- > 0)
		if (pit2_wait_us(1000u) != 0)
			return -1;
	return 0;
}

/* ---- local APIC (physical MMIO: called with paging off, or inside restart's temporary mapping) ---- */
static unsigned int lr(unsigned int off) { return amp_mmio_rd(lapic + off); }
static void lw(unsigned int off, unsigned int v) { amp_mmio_wr(lapic + off, v); }

static int icr_send(unsigned int dest, unsigned int lo)
{
	unsigned int i;

	for (i = 0; i < 1000000u && (lr(LAPIC_ICRLO) & 0x1000u); i++)
		;
	lw(LAPIC_ESR, 0);
	lw(LAPIC_ICRHI, dest << 24);
	lw(LAPIC_ICRLO, lo);
	for (i = 0; i < 1000000u; i++)
		if ((lr(LAPIC_ICRLO) & 0x1000u) == 0) {
			lw(LAPIC_ESR, 0);
			esr_seen |= lr(LAPIC_ESR);
			return 0;
		}
	return -1;
}

/* ---- processor inventory ---- */
static unsigned int sum8(const unsigned char *p, unsigned int n)
{
	unsigned int s = 0;

	while (n-- > 0)
		s += *p++;
	return s & 0xFFu;
}

static int sig(const unsigned char *p, const char *s)
{
	while (*s)
		if (*p++ != (unsigned char) *s++)
			return 0;
	return 1;
}

static unsigned int u32at(const unsigned char *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned int) p[3] << 24); }
static unsigned int u16at(const unsigned char *p) { return p[0] | (p[1] << 8); }

static const unsigned char *scan(unsigned int lo, unsigned int hi, const char *s, unsigned int len)
{
	unsigned int a;

	for (a = lo; a + len <= hi; a += 16u)
		if (sig((const unsigned char *) a, s) && sum8((const unsigned char *) a, len) == 0)
			return (const unsigned char *) a;
	return 0;
}

static int from_madt(void)
{
	const unsigned char *r = 0, *t, *h, *e, *end;
	unsigned int ebda = u16at((const unsigned char *) 0x40Eu) << 4, rsdt, len, n, i, l;

	if (ebda >= 0x80000u && ebda < 0xA0000u)
		r = scan(ebda, ebda + 1024u, "RSD PTR ", 20u);
	if (r == 0)
		r = scan(0xE0000u, 0x100000u, "RSD PTR ", 20u);
	if (r == 0)
		return -1;
	rsdt = u32at(r + 16);
	t = (const unsigned char *) rsdt;
	if (rsdt < 0x100000u || !sig(t, "RSDT"))
		return -1;
	len = u32at(t + 4);
	if (len < 36u || len > 0x10000u || sum8(t, len) != 0)
		return -1;
	n = (len - 36u) / 4u;
	for (i = 0; i < n; i++) {
		h = (const unsigned char *) u32at(t + 36 + 4 * i);
		if ((unsigned int) h < 0x100000u || !sig(h, "APIC"))
			continue;
		l = u32at(h + 4);
		if (l < 44u || l > 0x10000u || sum8(h, l) != 0)
			return -1;
		end = h + l;
		for (e = h + 44; e + 2 <= end && e[1] >= 2 && e + e[1] <= end; e += e[1])
			if (e[0] == 0 && e[1] >= 8 && (u32at(e + 4) & 1u) && ncpu < MAXCPU)
				cpu_ids[ncpu++] = e[3];
		method = 1;
		return 0;
	}
	return -1;
}

static int from_mptable(void)
{
	const unsigned char *f = 0, *c, *e, *end;
	unsigned int ebda = u16at((const unsigned char *) 0x40Eu) << 4, base = u16at((const unsigned char *) 0x413u) * 1024u;
	unsigned int len, cnt, i;

	if (ebda >= 0x80000u && ebda < 0xA0000u)
		f = scan(ebda, ebda + 1024u, "_MP_", 16u);
	if (f == 0 && base >= 0x80000u && base <= 0xA0000u)
		f = scan(base - 1024u, base, "_MP_", 16u);
	if (f == 0)
		f = scan(0xF0000u, 0x100000u, "_MP_", 16u);
	if (f == 0 || f[8] != 1 || f[11] != 0)            /* length 16 bytes; feature byte 1 != 0: default configs, refused */
		return -1;
	c = (const unsigned char *) u32at(f + 4);
	if (c == 0 || !sig(c, "PCMP"))
		return -1;
	len = u16at(c + 4);
	if (len < 44u || sum8(c, len) != 0)
		return -1;
	cnt = u16at(c + 34);
	end = c + len;
	e = c + 44;
	for (i = 0; i < cnt && e < end; i++) {
		if (e[0] == 0) {
			if (e + 20 > end)
				return -1;
			if ((e[3] & 1u) && ncpu < MAXCPU)
				cpu_ids[ncpu++] = e[1];
			e += 20;
		} else if (e[0] >= 1 && e[0] <= 4)
			e += 8;
		else
			return -1;                     /* unknown entry: the table cannot be walked safely */
	}
	method = 2;
	return 0;
}

/* ---- start the worker (boot, or restart after a positive halt acknowledgement) ---- */
static void prepare(void)
{
	unsigned char *d = (unsigned char *) AMP_TRAMP_PA, *s = amp_trampoline;
	struct amp_mbox *m = &amp_mbox;
	unsigned int i, off;

	while (s < amp_trampoline_end)
		*d++ = *s++;
	for (i = 0; i < 32u; i++) {
		off = ap_exc_table[i];
		amp_ap_idt[2 * i] = (0x08u << 16) | (off & 0xFFFFu);
		amp_ap_idt[2 * i + 1] = (off & 0xFFFF0000u) | 0x8E00u;
	}
	amp_ap_stack_top_v = ((unsigned int) amp_ap_stack + AMP_AP_STACK) & ~15u;
	m->ap_magic = m->ap_version = m->ap_apic_id = m->ap_cpuid_id = m->ap_boot_gen = m->ap_stage = 0;
	m->f_vec = m->f_err = m->f_eip = m->f_cr2 = m->f_esp = m->f_eflags = m->f_apic = 0;
	m->cancel_gen = 0;
	m->ap_ctl = AP_CTL_RUN;
	amp_xchg(&m->state, RQ_IDLE);
	amp_xchg(&m->ap_state, AP_ST_OFF);
	boots++;
	amp_xchg(&m->boot_gen, boots);
}

static int send_start(void)
{
	if (icr_send(ap_id, 0x00004500u) != 0)                       /* INIT, level assert */
		return AMP_R_ICR;
	if (wait_ms(10) != 0)
		return AMP_R_PIT;
	if (icr_send(ap_id, 0x00004600u | (AMP_TRAMP_PA >> 12)) != 0) /* STARTUP, vector 0x08 */
		return AMP_R_ICR;
	if (pit2_wait_us(200) != 0)
		return AMP_R_PIT;
	if (amp_load(&amp_mbox.ap_state) == AP_ST_OFF) {
		if (icr_send(ap_id, 0x00004600u | (AMP_TRAMP_PA >> 12)) != 0)
			return AMP_R_ICR;
		if (pit2_wait_us(200) != 0)
			return AMP_R_PIT;
	}
	return AMP_R_NONE;
}

static int identity_ok(void)
{
	struct amp_mbox *m = &amp_mbox;

	return amp_load(&m->ap_magic) == AMP_AP_MAGIC && amp_load(&m->ap_version) == AMP_ABI_VERSION &&
	       amp_load(&m->ap_boot_gen) == boots && amp_load(&m->ap_apic_id) == ap_id;
}

static void fail(int r)
{
	reason = r;
	svc = SVC_UNAVAIL;
	con_puts("AMP: worker service unavailable - ");
	con_puts(rname[r]);
	con_puts("\n");
}

void amp_boot(void)
{
	unsigned int r[4], hi, lo, i, ms;
	int rc;

	if (!amp_has_cpuid()) {
		fail(AMP_R_NOCPUID);
		return;
	}
	amp_cpuid(1, r);
	if ((r[3] & (1u << 9)) == 0 || (r[3] & (1u << 5)) == 0) {
		fail(AMP_R_NOAPIC);                /* no local APIC, or no RDMSR to read IA32_APIC_BASE */
		return;
	}
	lo = amp_rdmsr(0x1Bu, &hi);
	if ((lo & (1u << 11)) == 0 || hi != 0) {
		fail(AMP_R_APICOFF);
		return;
	}
	lapic = lo & 0xFFFFF000u;
	bsp_id = lr(LAPIC_ID) >> 24;
	lapic_ver = lr(LAPIC_VER);
	lapic_svr = lr(LAPIC_SVR);
	if (from_madt() != 0 && (ncpu = 0, from_mptable() != 0)) {
		fail(AMP_R_NOTABLES);
		return;
	}
	con_puts(method == 1 ? "AMP: ACPI MADT: " : "AMP: MP table: ");
	con_putdec(ncpu);
	con_puts(" enabled processor(s), BSP LAPIC id ");
	con_putdec(bsp_id);
	con_puts("\n");
	for (i = 0; i < (unsigned int) ncpu; i++)
		if (cpu_ids[i] != bsp_id)
			break;
	if (ncpu < 2 || i >= (unsigned int) ncpu) {
		fail(AMP_R_ONECPU);
		return;
	}
	ap_id = cpu_ids[i];
	prepare();
#ifdef QDOS_AMP_TEST_NOSIPI
	rc = AMP_R_NONE;                       /* TEST-ONLY: the start-up IPIs are never sent (a missing worker) */
#else
	rc = send_start();
#endif
	if (rc != AMP_R_NONE) {
		fail(rc);
		return;
	}
	for (ms = 0; ms < 200u && amp_load(&amp_mbox.ap_state) != AP_ST_READY; ms++)
		if (wait_ms(1) != 0) {
			fail(AMP_R_PIT);
			return;
		}
	if (amp_load(&amp_mbox.ap_state) != AP_ST_READY) {
		fail(AMP_R_TIMEOUT);
		return;
	}
	if (!identity_ok()) {
		fail(AMP_R_IDENTITY);
		return;
	}
	ready_us = ms;
	svc = SVC_READY;
	con_puts("AMP: worker CPU LAPIC id ");
	con_putdec(ap_id);
	con_puts(" READY (boot ");
	con_putdec(boots);
	con_puts(", within ");
	con_putdec(ms + 1u);
	con_puts(" ms); CPU 0 keeps the kernel\n");
}

/* ---- service ---- */
static int owner_alive(void)
{
	unsigned int s = owner & 0xFFFFu;

	return s < (unsigned int) JOB_MAX && jobs[s].in_use && !jobs[s].dead && jobs[s].id == owner;
}

static void refresh(void)
{
	unsigned int as = amp_load(&amp_mbox.ap_state), st;

	if (svc == SVC_STARTING && as == AP_ST_READY)
		svc = identity_ok() ? SVC_READY : (reason = AMP_R_IDENTITY, SVC_UNAVAIL);
	else if (svc == SVC_READY && as == AP_ST_FAULTED)
		svc = SVC_FAULTED;
	else if (svc == SVC_READY && as == AP_ST_QUIESCED)
		svc = SVC_QUIESCED;
	if (svc == SVC_UNAVAIL && reason == AMP_R_TIMEOUT && as == AP_ST_READY)
		late_seen = 1;                         /* reported, never adopted */
	st = amp_load(&amp_mbox.state);
	if (st != RQ_IDLE && owner != 0 && !owner_alive()) {
		if (st == RQ_DONE || st == RQ_ERROR || st == RQ_CANCELLED || svc == SVC_FAULTED || svc == SVC_QUIESCED) {
			amp_xchg(&amp_mbox.state, RQ_IDLE);    /* the worker has published, or has positively halted */
			owner = 0;
			orphans++;
		} else if (amp_load(&amp_mbox.cancel_gen) != cur_gen) {
			amp_xchg(&amp_mbox.cancel_gen, cur_gen);   /* pending until the worker acknowledges */
			cancels++;
		}
	}
}

/* A job has ended (exit, kill, fault): if it owns the outstanding request, ask the worker to stop at once.  The slot is
 * NOT freed here - only refresh() frees it, after the worker has published DONE / ERROR / CANCELLED. */
void amp_job_ended(unsigned int id)
{
	unsigned int st;

	if (owner == 0 || owner != id)
		return;
	st = amp_load(&amp_mbox.state);
	if ((st == RQ_QUEUED || st == RQ_RUNNING) && amp_load(&amp_mbox.cancel_gen) != cur_gen) {
		amp_xchg(&amp_mbox.cancel_gen, cur_gen);
		cancels++;
	}
}

static int mine(unsigned int gen)
{
	if (owner == 0 || gen != cur_gen)
		return AMP_ERR_STALE;
	if (owner != jobs[job_current].id)
		return AMP_ERR_OWNER;
	return 0;
}

static void kcopy(unsigned char *d, const unsigned char *s, unsigned int n)
{
	while (n-- > 0)
		*d++ = *s++;
}

static unsigned int publish(unsigned int cmd, unsigned int in_len, unsigned int out_max, unsigned int a0, unsigned int a1)
{
	struct amp_mbox *m = &amp_mbox;

	cur_gen = next_gen++;
	if (next_gen == 0)
		next_gen = 1;                          /* generation 0 is never used */
	owner = jobs[job_current].id;
	m->gen = cur_gen;
	m->cmd = cmd;
	m->in_len = in_len;
	m->out_max = out_max;
	m->arg0 = a0;
	m->arg1 = a1;
	m->done_gen = 0;
	amp_xchg(&m->cancel_gen, 0);
	amp_xchg(&m->state, RQ_QUEUED);           /* publication: all of the above is visible first */
	return cur_gen;
}

static void release(void)
{
	amp_xchg(&amp_mbox.state, RQ_IDLE);
	owner = 0;
}

static int restart(void)
{
#ifdef QDOS_PAGING
	unsigned int was, tpl, *dir, *pt, pde, rc;

	was = pg_enter_template();
	tpl = pg_read_cr3();
	dir = (unsigned int *) tpl;
	pt = (unsigned int *) (((unsigned int) amp_pt_mem + 4095u) & ~4095u);
	pde = lapic >> 22;
	if (dir[pde] != 0) {
		pg_leave_template(was);
		return AMP_ERR_STATE;                  /* something is mapped there already: do not touch it */
	}
	pt[(lapic >> 12) & 0x3FFu] = lapic | 0x01u | 0x02u | 0x08u | 0x10u;   /* present, rw, PWT, PCD */
	dir[pde] = (unsigned int) pt | 0x01u | 0x02u;
	pg_load_cr3(tpl);
	prepare();
	rc = send_start();
	dir[pde] = 0;
	pt[(lapic >> 12) & 0x3FFu] = 0;
	pg_load_cr3(tpl);
	pg_leave_template(was);
	if (rc != AMP_R_NONE) {
		reason = (int) rc;
		svc = SVC_UNAVAIL;
		return AMP_ERR_STATE;
	}
	svc = SVC_STARTING;
	return 0;
#else
	return AMP_ERR_STATE;
#endif
}

int amp_trap(unsigned int a, unsigned int b, unsigned int c)
{
	struct amp_mbox *m = &amp_mbox;
	struct amp_submit s;
	struct amp_result r;
	struct amp_proof p;
	unsigned int w[AMP_INFO_WORDS], st, n, i;
	unsigned long t0;
	int rc;

	refresh();
	switch (a) {
	case AMP_OP_INFO:
		if (!chan_user_range_ok(c, sizeof w))
			return AMP_ERR_RANGE;
		for (i = 0; i < AMP_INFO_WORDS; i++)
			w[i] = 0;
		w[0] = AMP_ABI_VERSION; w[1] = (unsigned int) svc; w[2] = (unsigned int) reason; w[3] = (unsigned int) method;
		w[4] = (unsigned int) ncpu; w[5] = bsp_id; w[6] = ap_id; w[7] = amp_load(&m->ap_apic_id);
		w[8] = amp_load(&m->ap_cpuid_id); w[9] = amp_load(&m->heartbeat); w[10] = amp_load(&m->ap_completed);
		w[11] = amp_load(&m->ap_errors); w[12] = amp_load(&m->state); w[13] = cur_gen; w[14] = owner; w[15] = next_gen;
		w[16] = m->f_vec; w[17] = m->f_err; w[18] = m->f_eip; w[19] = m->f_cr2; w[20] = m->f_esp; w[21] = m->f_apic;
		w[22] = boots; w[23] = (unsigned int) late_seen; w[24] = orphans; w[25] = cancels;
		w[26] = amp_load(&m->burn_iter); w[27] = amp_load(&m->burn_bad); w[28] = amp_load(&m->ap_state);
		w[29] = lapic; w[30] = lapic_ver; w[31] = esr_seen;
		kcopy((unsigned char *) c, (const unsigned char *) w, sizeof w);
		return 0;
	case AMP_OP_SUBMIT:
		if (svc != SVC_READY)
			return AMP_ERR_UNAVAIL;
		if (!chan_user_range_ok(c, sizeof s))
			return AMP_ERR_RANGE;
		kcopy((unsigned char *) &s, (const unsigned char *) c, sizeof s);
		if (s.in_len > AMP_BUF_BYTES || s.out_max > AMP_BUF_BYTES || (s.in_len != 0 && !chan_user_range_ok(s.in_ptr, s.in_len)))
			return AMP_ERR_RANGE;
		if (amp_load(&m->state) != RQ_IDLE)
			return AMP_ERR_BUSY;
		kcopy(amp_in, (const unsigned char *) s.in_ptr, s.in_len);
		s.gen = publish(s.cmd, s.in_len, s.out_max, s.arg0, s.arg1);
		((struct amp_submit *) c)->gen = s.gen;
		return 0;
	case AMP_OP_POLL:
		if ((rc = mine(b)) != 0)
			return rc;
		st = amp_load(&m->state);
		if ((st == RQ_QUEUED || st == RQ_RUNNING) && svc == SVC_FAULTED)
			return AMP_ERR_FAULT;
		return (int) st;
	case AMP_OP_RESULT:
		if ((rc = mine(b)) != 0)
			return rc;
		if (!chan_user_range_ok(c, sizeof r))
			return AMP_ERR_RANGE;
		kcopy((unsigned char *) &r, (const unsigned char *) c, sizeof r);
		st = amp_load(&m->state);
		if ((st == RQ_QUEUED || st == RQ_RUNNING) && (svc == SVC_FAULTED || svc == SVC_QUIESCED)) {
			release();                         /* the worker has positively halted: the slot is free */
			return AMP_ERR_FAULT;
		}
		if (st != RQ_DONE && st != RQ_ERROR && st != RQ_CANCELLED)
			return AMP_ERR_NOTREADY;
		if (amp_load(&m->done_gen) != b) {
			release();                         /* published (the worker is done with it), but not our answer */
			return AMP_ERR_STALE;
		}
		n = m->out_len;
		if (n > AMP_BUF_BYTES || n > r.out_max || (n != 0 && !chan_user_range_ok(r.out_ptr, n)))
			return AMP_ERR_RANGE;
		kcopy((unsigned char *) r.out_ptr, amp_out, n);
		r.state = st; r.status = m->status; r.out_len = n; r.r0 = m->r0; r.r1 = m->r1; r.gen = b;
		kcopy((unsigned char *) c, (const unsigned char *) &r, sizeof r);
		release();
		return 0;
	case AMP_OP_CANCEL:
		if ((rc = mine(b)) != 0)
			return rc;
		amp_xchg(&m->cancel_gen, b);
		cancels++;
		return 0;
	case AMP_OP_TEST:
		switch (b) {
		case AMP_T_SETGEN:
			if (amp_load(&m->state) != RQ_IDLE || c == 0)
				return AMP_ERR_BUSY;
			next_gen = c;
			return 0;
		case AMP_T_MALFORMED:
			if (svc != SVC_READY)
				return AMP_ERR_UNAVAIL;
			if (amp_load(&m->state) != RQ_IDLE)
				return AMP_ERR_BUSY;
			return (int) (publish(AMP_CMD_CRC32, AMP_BUF_BYTES + 4096u, 0, 0, 0) & 0x7FFFFFFFu);
		case AMP_T_QUIESCE:
			if (svc != SVC_READY)
				return AMP_ERR_UNAVAIL;
			amp_xchg(&m->ap_ctl, AP_CTL_QUIESCE);
			return 0;
		case AMP_T_RESTART:
			if (amp_load(&m->ap_state) != AP_ST_FAULTED && amp_load(&m->ap_state) != AP_ST_QUIESCED)
				return AMP_ERR_STATE;          /* only after a positive halt acknowledgement, never on a timeout */
			if (amp_load(&m->state) != RQ_IDLE)
				release();
			return restart();
		case AMP_T_PROOF:
			if (svc != SVC_READY)
				return AMP_ERR_UNAVAIL;
			if (!chan_user_range_ok(c, sizeof p))
				return AMP_ERR_RANGE;
			kcopy((unsigned char *) &p, (const unsigned char *) c, sizeof p);
			if (p.iters > AMP_NUM_MAX)
				return AMP_ERR_RANGE;
			if (amp_load(&m->state) != RQ_IDLE)
				return AMP_ERR_BUSY;
			/* Interrupts are off for the whole trap: CPU 0 runs nothing but this loop until the result is in. */
			p.hb_before = amp_load(&m->heartbeat);
			t0 = sched_ticks;
			p.ticks_before = (unsigned int) t0;
			p.gen = publish(AMP_CMD_NUMERIC, 0, 0, p.iters, p.seed);
			for (p.spins = 0; p.spins < 400000000u; p.spins++) {
				st = amp_load(&m->state);
				if (st == RQ_DONE || st == RQ_ERROR || st == RQ_CANCELLED)
					break;
				amp_pause();
			}
			p.hb_after = amp_load(&m->heartbeat);
			p.ticks_after = (unsigned int) sched_ticks;
			if (st == RQ_DONE && amp_load(&m->done_gen) == p.gen) {
				p.r0 = m->r0;
				p.r1 = m->r1;
				p.status = 0;
				release();
			} else
				p.status = (unsigned int) AMP_ERR_TIMEOUT;   /* the slot stays the caller's: nothing is reclaimed */
			kcopy((unsigned char *) c, (const unsigned char *) &p, sizeof p);
			return 0;
		}
		return AMP_ERR_RANGE;
	}
	return AMP_ERR_RANGE;
}
