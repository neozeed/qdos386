/*
 * QDOS/386 R14 - PC UART serial ports (docs/R14-SERIAL-ABI.md).
 *
 * Ports: SER1 = COM1 0x3F8 IRQ4 (vector 0x24), SER2 = COM2 0x2F8 IRQ3 (vector 0x23).  Probed once at boot with
 * interrupts off in the UART (bounded: scratch, IER readback, FIFO, one internal-loopback byte).
 *
 * Interrupt path (ser_irq3/ser_irq4, from the stubs in uart_s.s, IF clear): a bounded IIR loop; every pass reads
 * LSR, counts line errors, moves received bytes into the receive ring (dropped and counted when it is full), refills
 * the transmitter from the transmit ring, and turns the THRE interrupt off when the ring is empty.  It never
 * allocates, never schedules and wakes at most the one job waiting on the port.  EOI to the master PIC.
 *
 * Waiting: QT_SER SER_OP_WAIT runs with IF clear.  If nothing is ready it records the waiter, the flags and a
 * deadline, sets the job's status to -1 (indefinite, so a console key can wake it through the console waiter) with
 * jobs[].hold pointing at the port's hold byte, and the trap schedules.  A wake from anywhere writes 0xFF to the
 * hold byte (HOLD_RELEASE), which is how a stale registration is recognised.  ser_tick (timer interrupt) ends
 * expired waits and waits for "transmitter empty", which raises no interrupt of its own.
 */
#ifdef QDOS_R14
#include "uart.h"
#include "channel.h"
#include "jobs.h"
#include "vcon.h"
#include "taskmgr.h"
#include "platform.h"

extern void outb(unsigned int port, unsigned int val);
extern unsigned int inb(unsigned int port);
/* job_current and jobs[] come from jobs.h (R16 makes job_current a per-CPU macro, so no local extern). */

struct ser_port {
	unsigned int base, irq;
	int type;
	int open;
	unsigned int owner;
	unsigned int fifo;                     /* bytes per THRE refill */
	unsigned int ier, mcr;
	struct ser_cfg cfg;
	unsigned long def_baud;
	unsigned char rx[SER_RXSZ];
	volatile unsigned int rh, rt;          /* head (ISR writes), tail (reader) */
	unsigned char tx[SER_TXSZ];
	volatile unsigned int th, tt;          /* head (writer), tail (ISR) */
	int waiter;                            /* job slot or -1 */
	unsigned int wflags;
	unsigned long deadline;
	int forever;
	volatile unsigned char hold;           /* the waiter's JOB.HOLD byte: 0xFF once it was woken */
	int cancel;
	int rts_low;
	struct ser_status st;
};

static struct ser_port ports[2];
static int ser_ready = 0;

static void bcopy_k(void *d, const void *s, unsigned int n)   /* no libc in the kernel: struct copies by hand */
{
	unsigned char *dd = (unsigned char *) d;
	const unsigned char *ss = (const unsigned char *) s;

	while (n-- != 0)
		*dd++ = *ss++;
}

static unsigned int rx_count(struct ser_port *p) { return (p->rh - p->rt) & (SER_RXSZ - 1); }
static unsigned int tx_count(struct ser_port *p) { return (p->th - p->tt) & (SER_TXSZ - 1); }

/* ---- probe ------------------------------------------------------------------------------------------------------ */

static int probe(unsigned int b)
{
	unsigned int lcr, mcr, s1, s2, ier, iir, got, i;

	lcr = inb(b + 3);
	outb(b + 3, lcr & 0x7f);
	mcr = inb(b + 4);
	outb(b + 7, 0x55); s1 = inb(b + 7);
	outb(b + 7, 0xAA); s2 = inb(b + 7);
	outb(b + 1, 0x00);
	outb(b + 1, 0x0F); ier = inb(b + 1);
	outb(b + 1, 0x00);
	if (ier != 0x0F) {
		outb(b + 3, lcr);
		return SER_T_ABSENT;
	}
	outb(b + 2, 0x07);
	iir = inb(b + 2);
	outb(b + 2, 0x00);
	outb(b + 3, 0x03);
	outb(b + 4, 0x10);                     /* internal loopback, outputs off */
	for (i = 0; i < 64 && (inb(b + 5) & 1); i++)
		(void) inb(b);
	outb(b, 0xA5);
	got = 0x100;
	for (i = 0; i < 100000; i++)
		if (inb(b + 5) & 1) {
			got = inb(b);
			break;
		}
	outb(b + 4, mcr & ~0x10u);
	outb(b + 3, lcr);
	if (got != 0xA5)
		return SER_T_ABSENT;
	if ((iir & 0xC0) == 0xC0)
		return SER_T_16550A;
	return (s1 == 0x55 && s2 == 0xAA) ? SER_T_16450 : SER_T_8250;
}

static const char *type_name(int t)
{
	return t == SER_T_16550A ? "16550A" : t == SER_T_16450 ? "16450" : t == SER_T_8250 ? "8250" : "absent";
}

void ser_boot(void)
{
	int k;

	ports[0].base = 0x3F8; ports[0].irq = 4;
	ports[1].base = 0x2F8; ports[1].irq = 3;
	for (k = 0; k < 2; k++) {
		struct ser_port *p = &ports[k];

		p->type = probe(p->base);
		p->waiter = -1;
		p->def_baud = 9600;
		p->st.type = (unsigned long) p->type;
		con_puts(k == 0 ? "SERIAL: SER1 COM1 0x3f8 IRQ4 " : "SERIAL: SER2 COM2 0x2f8 IRQ3 ");
		con_puts(type_name(p->type));
		con_puts("\n");
	}
	ser_ready = 1;
}

/* ---- the interrupt path ----------------------------------------------------------------------------------------- */

static unsigned int ready(struct ser_port *p, unsigned int flags)
{
	unsigned int m = 0;

	if (rx_count(p) != 0)
		m |= SER_RX;
	if (tx_count(p) < SER_TXSZ - 1)
		m |= SER_TXSPACE;
	if (tx_count(p) == 0 && (inb(p->base + 5) & 0x40))
		m |= SER_TXEMPTY;
	if ((flags & SER_KEY) && p->waiter >= 0) {
		int cn = vcon_job_con(p->waiter);
		if (vcon_count(cn) != 0)
			m |= SER_KEY;
	}
	return m & flags;
}

static void unwait(struct ser_port *p)
{
	int w = p->waiter;
	int cn;

	if (w < 0)
		return;
	p->waiter = -1;
	cn = vcon_job_con(w);
	if (vcon_waiter(cn) == w)
		vcon_set_waiter(cn, -1);
}

static void wake(struct ser_port *p)
{
	int w = p->waiter;

	if (w < 0)
		return;
	unwait(p);
	if (p->hold == 0)                      /* still asleep in our wait */
		job_wake(w);
}

static void handshake(struct ser_port *p)
{
	unsigned int n;

	if (p->cfg.handshake != 'H')
		return;
	n = rx_count(p);
	if (!p->rts_low && n >= (SER_RXSZ * 3) / 4) {
		p->rts_low = 1;
		p->mcr &= ~0x02u;
		outb(p->base + 4, p->mcr);
	} else if (p->rts_low && n <= SER_RXSZ / 4) {
		p->rts_low = 0;
		p->mcr |= 0x02u;
		outb(p->base + 4, p->mcr);
	}
}

static void tx_fill(struct ser_port *p)
{
	unsigned int n = 0;

	if (p->cfg.handshake == 'H' && !(p->st.msr & 0x10))
		return;                            /* CTS low: the modem-status interrupt restarts us */
	while (n < p->fifo && tx_count(p) != 0) {
		outb(p->base, p->tx[p->tt]);
		p->tt = (p->tt + 1) & (SER_TXSZ - 1);
		p->st.tx_bytes++;
		n++;
	}
	if (tx_count(p) == 0 && (p->ier & 0x02)) {
		p->ier &= ~0x02u;
		outb(p->base + 1, p->ier);
	}
}

static void service(struct ser_port *p)
{
	unsigned int lsr, c, k;

	for (k = 0; k < 64; k++) {
		lsr = inb(p->base + 5);
		p->st.lsr = lsr;
		if (lsr & 0x02)
			p->st.overrun++;
		if (!(lsr & 0x01))
			break;
		c = inb(p->base);
		if (lsr & 0x10) {
			p->st.breaks++;                /* the NUL delivered with a break is not data */
			continue;
		}
		if (lsr & 0x04)
			p->st.parity++;
		if (lsr & 0x08)
			p->st.framing++;
		p->st.rx_bytes++;
		if (rx_count(p) == SER_RXSZ - 1) {
			p->st.rx_dropped++;
			continue;
		}
		p->rx[p->rh] = (unsigned char) c;
		p->rh = (p->rh + 1) & (SER_RXSZ - 1);
	}
	handshake(p);
	if ((lsr & 0x20) && tx_count(p) != 0)
		tx_fill(p);
	else if (tx_count(p) == 0 && (p->ier & 0x02)) {
		p->ier &= ~0x02u;
		outb(p->base + 1, p->ier);
	}
}

static void irq(struct ser_port *p)
{
	unsigned int iir, k;

	p->st.irqs++;
	if (p->open) {
		for (k = 0; k < 16; k++) {
			iir = inb(p->base + 2);
			if ((iir & 0x0E) == 0x00 && !(iir & 1))
				p->st.msr = inb(p->base + 6);
			service(p);
			if (iir & 1)
				break;
		}
		if (p->waiter >= 0 && ready(p, p->wflags) != 0)
			wake(p);
	} else {
		outb(p->base + 1, 0);              /* not open: silence the port */
	}
	outb(0x20, 0x20);
}

void ser_irq3(void) { irq(&ports[1]); }
void ser_irq4(void) { irq(&ports[0]); }

void ser_tick(void)
{
	int k;

	if (!ser_ready)
		return;
	for (k = 0; k < 2; k++) {
		struct ser_port *p = &ports[k];

		if (p->waiter < 0)
			continue;
		if (p->hold != 0) {                /* woken elsewhere (a key, KILL): forget the registration */
			unwait(p);
			continue;
		}
		if (ready(p, p->wflags) != 0 || (!p->forever && (long) (sched_ticks - p->deadline) >= 0))
			wake(p);
	}
}

/* ---- configuration ---------------------------------------------------------------------------------------------- */

static int cfg_ok(const struct ser_cfg *c)
{
	if (c->baud == 0 || c->baud > 115200UL || (115200UL % c->baud) != 0)
		return 0;
	if (c->data_bits < 5 || c->data_bits > 8 || (c->stop_bits != 1 && c->stop_bits != 2))
		return 0;
	if (c->parity != 'N' && c->parity != 'O' && c->parity != 'E' && c->parity != 'M' && c->parity != 'S')
		return 0;
	return c->handshake == 'I' || c->handshake == 'H';
}

static void apply(struct ser_port *p)
{
	unsigned int div = (unsigned int) (115200UL / p->cfg.baud);
	unsigned int lcr = (unsigned int) (p->cfg.data_bits - 5) | (p->cfg.stop_bits == 2 ? 0x04u : 0u);

	switch (p->cfg.parity) {
	case 'O': lcr |= 0x08; break;
	case 'E': lcr |= 0x18; break;
	case 'M': lcr |= 0x28; break;
	case 'S': lcr |= 0x38; break;
	}
	outb(p->base + 3, 0x80 | lcr);
	outb(p->base + 0, div & 0xff);
	outb(p->base + 1, div >> 8);
	outb(p->base + 3, lcr);
	p->ier = 0x01 | 0x04 | (p->cfg.handshake == 'H' ? 0x08u : 0u) | (p->ier & 0x02u);
	outb(p->base + 1, p->ier);
	p->st.msr = inb(p->base + 6);
	p->rts_low = 0;
	p->mcr = 0x0B;                         /* DTR, RTS, OUT2 (the PC's IRQ enable) */
	outb(p->base + 4, p->mcr);
}

/* ---- the SER channel driver ------------------------------------------------------------------------------------- */

static int upper(int c) { return (c >= 'a' && c <= 'z') ? c - 32 : c; }

static int ser_open(struct chan_object *ch, const char *name)
{
	struct ser_port *p;
	int n, i, par = 'N', hs = 'I', k;

	/* "SER" + port + [O|E|M|S] [H|I] [R]; Z and C (translation) are refused */
	if (name[3] != '1' && name[3] != '2')
		return CHAN_ERR_NAME;
	n = name[3] - '1';
	for (i = 4; name[i] != 0; i++) {
		k = upper((unsigned char) name[i]);
		if ((k == 'O' || k == 'E' || k == 'M' || k == 'S') && par == 'N' && hs == 'I' && i == 4)
			par = k;
		else if ((k == 'H' || k == 'I') && (i == 4 || (i == 5 && par != 'N')))
			hs = k;
		else if (k == 'R' && name[i + 1] == 0)
			;
		else if ((k == 'Z' || k == 'C') && name[i + 1] == 0)
			return SER_ERR_BADPARM;
		else
			return CHAN_ERR_NAME;
	}
	p = &ports[n];
	if (!ser_ready || p->type == SER_T_ABSENT)
		return CHAN_ERR_NAME;              /* not found: no such port on this machine */
	if (p->open)
		return SER_ERR_INUSE;
	p->open = 1;
	p->owner = ch->owner;
	ch->ser = n;
	p->rh = p->rt = p->th = p->tt = 0;
	p->cancel = 0;
	p->waiter = -1;
	k = p->type;
	{
		unsigned long irqs = p->st.irqs;
		unsigned int z;
		for (z = 0; z < sizeof p->st / sizeof(unsigned long); z++)
			((unsigned long *) &p->st)[z] = 0;
		p->st.irqs = irqs;
		p->st.type = (unsigned long) k;
	}
	p->cfg.baud = p->def_baud;
	p->cfg.data_bits = par == 'N' ? 8 : 7;
	p->cfg.parity = (unsigned char) par;
	p->cfg.stop_bits = 1;
	p->cfg.handshake = (unsigned char) hs;
	p->fifo = (p->type == SER_T_16550A) ? 16 : 1;
	outb(p->base + 1, 0);
	if (p->type == SER_T_16550A)
		outb(p->base + 2, 0x87);           /* FIFO on, both cleared, receive trigger 8 */
	p->ier = 0;
	apply(p);
	for (i = 0; i < 64 && (inb(p->base + 5) & 1); i++)
		(void) inb(p->base);
	(void) inb(p->base + 2);
	outb(0x21, inb(0x21) & ~(1u << p->irq));
	return 0;
}

static void ser_close(struct chan_object *ch)
{
	struct ser_port *p = &ports[ch->ser];

	outb(p->base + 1, 0);
	p->ier = 0;
	outb(p->base + 4, 0);                  /* DTR, RTS, OUT2 off */
	if (p->type == SER_T_16550A)
		outb(p->base + 2, 0x07);
	outb(0x21, inb(0x21) | (1u << p->irq));
	p->st.tx_discarded += tx_count(p);
	p->th = p->tt;
	p->open = 0;
	wake(p);                               /* a waiter sees the channel gone */
}

static int ser_read(struct chan_object *ch, unsigned char *buf, unsigned int n)
{
	struct ser_port *p = &ports[ch->ser];
	unsigned int k = 0;

	while (k < n && rx_count(p) != 0) {
		buf[k++] = p->rx[p->rt];
		p->rt = (p->rt + 1) & (SER_RXSZ - 1);
	}
	handshake(p);
	return k != 0 ? (int) k : CHAN_ERR_AGAIN;
}

static int ser_write(struct chan_object *ch, const unsigned char *buf, unsigned int n)
{
	struct ser_port *p = &ports[ch->ser];
	unsigned int k = 0;

	while (k < n && tx_count(p) < SER_TXSZ - 1) {
		p->tx[p->th] = buf[k++];
		p->th = (p->th + 1) & (SER_TXSZ - 1);
	}
	if (k != 0) {
		if (inb(p->base + 5) & 0x20)
			tx_fill(p);
		if (tx_count(p) != 0 && !(p->ier & 0x02)) {
			p->ier |= 0x02;
			outb(p->base + 1, p->ier);
		}
	}
	return k != 0 ? (int) k : CHAN_ERR_AGAIN;
}

static int ser_ctl(struct chan_object *ch, unsigned int op, unsigned long arg)
{
	(void) ch;
	(void) arg;
	return op == CHAN_CTL_FLUSH ? 0 : CHAN_ERR_NOTSUP;
}

struct chan_driver ser_driver = { "SER", 1, ser_open, ser_close, ser_write, ser_read, 0, ser_ctl, 0 };

/* ---- QT_SER ----------------------------------------------------------------------------------------------------- */

static int port_of(unsigned int id, struct ser_port **out)
{
	struct chan_object *ch;
	int rc = chan_lookup(id, job_current_id(), &ch);

	if (rc != 0)
		return rc;
	if (ch->drv != &ser_driver)
		return CHAN_ERR_NOTSUP;
	*out = &ports[ch->ser];
	return 0;
}

int ser_trap(unsigned int a, unsigned int b, unsigned int c, unsigned int *eax)
{
	struct ser_port *p;
	int rc, k;
	unsigned int m;

	if (!ser_ready)
		return (int) (*eax = (unsigned int) CHAN_ERR_NOTSUP), 0;
	/* a registration left by an earlier wait of this job that ended without us (timeout seen by the job itself,
	 * a key, or the job's own poll) is dropped here: the job is running, so it is not waiting. */
	for (k = 0; k < 2; k++)
		if (ports[k].waiter == job_current)
			unwait(&ports[k]);

	switch (a) {
	case SER_OP_PROBE:
		*eax = (b == 1 || b == 2) ? (unsigned int) ports[b - 1].type : (unsigned int) CHAN_ERR_NAME;
		return 0;
	case SER_OP_CANCEL:
		if (b != 1 && b != 2) {
			*eax = (unsigned int) CHAN_ERR_NAME;
			return 0;
		}
		p = &ports[b - 1];
		if (p->open) {
			p->cancel = 1;
			wake(p);
		}
		*eax = 0;
		return 0;
	case SER_OP_DEFAULT:
		if (b != 1 && b != 2) {
			*eax = (unsigned int) CHAN_ERR_NAME;
			return 0;
		}
		if (c == 0)
			c = 9600;
		if (c > 115200u || (115200u % c) != 0) {
			*eax = (unsigned int) SER_ERR_BADPARM;
			return 0;
		}
		ports[b - 1].def_baud = c;
		*eax = 0;
		return 0;
	}

	rc = port_of(b, &p);
	if (rc != 0) {
		*eax = (unsigned int) rc;
		return 0;
	}
	switch (a) {
	case SER_OP_CONFIG: {
		struct ser_cfg nc;
		if (!chan_user_range_ok(c, sizeof nc)) {
			*eax = (unsigned int) CHAN_ERR_RANGE;
			return 0;
		}
		bcopy_k(&nc, (const void *) c, sizeof nc);
		if (!cfg_ok(&nc)) {
			*eax = (unsigned int) SER_ERR_BADPARM;
			return 0;
		}
		bcopy_k(&p->cfg, &nc, sizeof nc);
		apply(p);
		*eax = 0;
		return 0;
	}
	case SER_OP_GETCFG:
		if (!chan_user_range_ok(c, sizeof(struct ser_cfg))) {
			*eax = (unsigned int) CHAN_ERR_RANGE;
			return 0;
		}
		bcopy_k((void *) c, &p->cfg, sizeof p->cfg);
		*eax = 0;
		return 0;
	case SER_OP_STATUS:
		if (!chan_user_range_ok(c, sizeof(struct ser_status))) {
			*eax = (unsigned int) CHAN_ERR_RANGE;
			return 0;
		}
		p->st.rx_queued = rx_count(p);
		p->st.tx_queued = tx_count(p);
		p->st.msr = inb(p->base + 6);
		p->st.rts = (p->mcr & 0x02) ? 1 : 0;
		bcopy_k((void *) c, &p->st, sizeof p->st);
		*eax = 0;
		return 0;
	case SER_OP_FLUSH:
		if (c & SER_FLUSH_RX)
			p->rt = p->rh;
		if (c & SER_FLUSH_TX) {
			p->st.tx_discarded += tx_count(p);
			p->th = p->tt;
		}
		handshake(p);
		*eax = 0;
		return 0;
	case SER_OP_POLL:
		if (p->cancel) {
			p->cancel = 0;
			p->st.cancels++;
			*eax = (unsigned int) SER_ERR_CANCEL;
			return 0;
		}
		p->waiter = job_current;           /* for the console check only */
		m = ready(p, c & 0x0F);
		p->waiter = -1;
		*eax = m;
		return 0;
	case SER_OP_WAIT: {
		unsigned int flags = (c >> 24) & 0x0F, ticks = c & 0xFFFFFFu;
		int cn, w;

		if (p->cancel) {
			p->cancel = 0;
			p->st.cancels++;
			*eax = (unsigned int) SER_ERR_CANCEL;
			return 0;
		}
		if (flags == 0) {
			*eax = (unsigned int) CHAN_ERR_RANGE;
			return 0;
		}
		p->waiter = job_current;
		m = ready(p, flags);
		if (m != 0 || ticks == 0) {
			p->waiter = -1;
			*eax = m != 0 ? m : (unsigned int) SER_ERR_TIMEOUT;
			return 0;
		}
		p->wflags = flags;
		p->forever = (ticks == SER_TICKS_FOREVER);
		p->deadline = sched_ticks + ticks;
		p->hold = 0;
		if (flags & SER_KEY) {
			cn = vcon_job_con(job_current);
			w = vcon_waiter(cn);
			if (w < 0 || w == job_current || !jobs[w].in_use || jobs[w].status >= 0)
				vcon_set_waiter(cn, job_current);
		}
		jobs[job_current].status = -1;
		jobs[job_current].hold = (unsigned char *) &p->hold;
		*eax = 0;
		return 1;                          /* the caller schedules */
	}
	}
	*eax = (unsigned int) CHAN_ERR_NOTSUP;
	return 0;
}
#endif
