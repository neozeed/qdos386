/*
 * QDOS/386 R15 AMP0 - the WORKER CPU's code and the memory it owns (docs/R15-AMP-ARCHITECTURE.md).
 *
 * Runs ONLY on the application processor: flat protected mode, paging off, IF clear, its own stack, GDT and IDT
 * (kernel/amp_ap_s.s).  It calls nothing outside this file and amp_ap_s.s (checked by tests/r15/check-ap-objects.py):
 * no QDOS service, no console, no allocation, no device.  It reads and writes only amp_mbox, amp_in, amp_out and its
 * stack - all static kernel BSS that CPU 0 never frees or remaps.
 *
 * Loop: count a heartbeat; obey ap_ctl (quiesce = acknowledge and halt); claim a QUEUED request with lock cmpxchg,
 * validate it again (the BSP checked it, but the worker trusts no length), run it, publish the result fields and then
 * the new state with xchg.  NUMERIC checks cancel_gen every 65536 iterations (cooperative cancellation).
 */
#include "amp.h"

unsigned int ap_load(volatile unsigned int *p);
unsigned int ap_xchg(volatile unsigned int *p, unsigned int v);
unsigned int ap_cas(volatile unsigned int *p, unsigned int old, unsigned int nw);
void ap_pause(void);
unsigned int ap_cpuid1_ebx(void);
unsigned int ap_rdmsr_lo(unsigned int msr);
unsigned int ap_read_cr2(void);
void ap_halt_forever(void);
void ap_invalid_op(void);

struct amp_mbox amp_mbox;
unsigned char amp_in[AMP_BUF_BYTES];
unsigned char amp_out[AMP_BUF_BYTES];
unsigned int amp_ap_stack[AMP_AP_STACK / 4];

static unsigned int ap_crc32(const unsigned char *p, unsigned int n, unsigned int *sum)
{
	unsigned int c = 0xFFFFFFFFu, s = 0, i;
	int k;

	for (i = 0; i < n; i++) {
		c ^= p[i];
		s += p[i];
		for (k = 0; k < 8; k++)
			c = (c & 1u) ? (c >> 1) ^ 0xEDB88320u : (c >> 1);
	}
	*sum = s;
	return c ^ 0xFFFFFFFFu;
}

static void ap_run(struct amp_mbox *m)
{
	unsigned int gen = m->gen, cmd = m->cmd, n = m->in_len, omax = m->out_max, a0 = m->arg0, a1 = m->arg1;
	unsigned int st = RQ_DONE, status = AMP_S_OK, olen = 0, r0 = 0, r1 = 0, i, x, s;

	if (n > AMP_BUF_BYTES || omax > AMP_BUF_BYTES) {
		st = RQ_ERROR;
		status = AMP_S_BOUNDS;
	} else if (cmd == AMP_CMD_CRC32) {
		r0 = ap_crc32(amp_in, n, &r1);
	} else if (cmd == AMP_CMD_XFORM) {
		if (n > omax) {
			st = RQ_ERROR;
			status = AMP_S_BOUNDS;
		} else {
			s = a0;
			for (i = 0; i < n; i++) {
				s = s * 1103515245u + 12345u;
				amp_out[n - 1u - i] = (unsigned char) (amp_in[i] ^ ((s >> 16) & 0xFFu));
			}
			olen = n;
			r0 = s;
		}
	} else if (cmd == AMP_CMD_NUMERIC || cmd == AMP_CMD_T_SLOW) {
		if (a0 > AMP_NUM_MAX) {
			st = RQ_ERROR;
			status = AMP_S_BOUNDS;
		} else {
			x = a1 ? a1 : 1u;
			s = 0;
			for (i = 0; i < a0; i++) {
				x ^= x << 13;
				x ^= x >> 17;
				x ^= x << 5;
				s += x ^ i;
				if (cmd == AMP_CMD_NUMERIC && (i & 0xFFFFu) == 0xFFFFu && ap_load(&m->cancel_gen) == gen) {
					st = RQ_CANCELLED;
					status = AMP_S_CANCELLED;
					i++;
					break;
				}
			}
			r0 = x;
			r1 = (st == RQ_CANCELLED) ? i : s;
		}
	} else if (cmd == AMP_CMD_T_STALE) {
		gen ^= 0x80000000u;               /* deliberately answer another generation */
	} else if (cmd == AMP_CMD_T_FAULT) {
		ap_invalid_op();                  /* #UD: ap_fault records it and halts this CPU */
	} else {
		st = RQ_ERROR;
		status = AMP_S_BADCMD;
	}
	m->status = status;
	m->out_len = olen;
	m->r0 = r0;
	m->r1 = r1;
	m->done_gen = gen;
	if (st == RQ_DONE)
		m->ap_completed++;
	else
		m->ap_errors++;
	ap_xchg(&m->state, st);               /* publish: everything above is visible before the new state */
}

#ifdef QDOS_AMP_TEST_BURN
/* TEST-ONLY kernels: between mailbox polls the worker keeps computing a CRC over its own 4 KiB pattern and checks it
 * against the value computed at start, so OS regression suites run while the worker computes. */
static unsigned char burn_buf[4096];
static unsigned int burn_ref;
#endif

void ap_main(void)
{
	struct amp_mbox *m = &amp_mbox;
	unsigned int base = ap_rdmsr_lo(0x1Bu) & 0xFFFFF000u;
#ifdef QDOS_AMP_TEST_BURN
	unsigned int i, sum;

	for (i = 0; i < 4096u; i++)
		burn_buf[i] = (unsigned char) (i * 7u + 3u);
	burn_ref = ap_crc32(burn_buf, 4096u, &sum);
#endif
	m->ap_stage = AP_ST_ENTRY;
	m->ap_lapic_base = base;
	m->ap_apic_id = (*(volatile unsigned int *) (base + 0x20u)) >> 24;   /* paging is off: the LAPIC page is physical */
	m->ap_cpuid_id = ap_cpuid1_ebx() >> 24;
	m->ap_boot_gen = m->boot_gen;
	m->ap_version = AMP_ABI_VERSION;
	m->ap_magic = AMP_AP_MAGIC;
#ifdef QDOS_AMP_TEST_LATE
	{
		volatile unsigned int d;
		for (d = 0; d < QDOS_AMP_TEST_LATE; d++)
			;                             /* TEST-ONLY: a worker that reports READY far too late */
	}
#endif
	ap_xchg(&m->ap_state, AP_ST_READY);
	for (;;) {
		m->heartbeat++;
		if (ap_load(&m->ap_ctl) == AP_CTL_QUIESCE) {
			ap_xchg(&m->ap_state, AP_ST_QUIESCED);   /* last write: from here on only cli; hlt */
			ap_halt_forever();
		}
		if (ap_load(&m->state) == RQ_QUEUED && ap_cas(&m->state, RQ_QUEUED, RQ_RUNNING) == RQ_QUEUED)
			ap_run(m);
		else {
#ifdef QDOS_AMP_TEST_BURN
			if (ap_crc32(burn_buf, 4096u, &sum) != burn_ref)
				m->burn_bad++;
			m->burn_iter++;
#else
			ap_pause();
#endif
		}
	}
}

/* Exception on the worker: frame = edi esi ebp esp ebx edx ecx eax (pushal), vector, error, eip, cs, eflags.  Record
 * and halt THIS CPU (ap_exc_common jumps to ap_halt_forever); CPU 0's handlers are never involved. */
void ap_fault(unsigned int *f)
{
	struct amp_mbox *m = &amp_mbox;

	m->f_vec = f[8];
	m->f_err = f[9];
	m->f_eip = f[10];
	m->f_eflags = f[12];
	m->f_esp = (unsigned int) (f + 13);
	m->f_cr2 = ap_read_cr2();
	m->f_apic = m->ap_apic_id;
	ap_xchg(&m->ap_state, AP_ST_FAULTED);    /* positive acknowledgement: nothing runs on this CPU after this */
}
