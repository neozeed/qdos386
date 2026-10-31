/* QDOS/386 R14 - serial port wrappers (QT_SER 0x3C; docs/R14-SERIAL-ABI.md).  A separate member of libqdos.a, so
 * programs that do not use serial ports are unchanged.  Waiting is always done by the kernel (SER_OP_WAIT): no call
 * here spins. */
#include <qdos386.h>

#define OP_PROBE   0
#define OP_CONFIG  1
#define OP_GETCFG  2
#define OP_STATUS  3
#define OP_WAIT    4
#define OP_POLL    5
#define OP_CANCEL  6
#define OP_FLUSH   7
#define OP_DEFAULT 8

static int ser(unsigned int op, unsigned int b, unsigned int c)
{
	return (int) qtrap(QD_TRAP_SER, op, b, c);
}

int qd_ser_probe(int port) { return ser(OP_PROBE, (unsigned int) port, 0); }
int qd_ser_config(int ch, const struct qd_ser_cfg *cfg) { return ser(OP_CONFIG, (unsigned int) ch, (unsigned int) cfg); }
int qd_ser_getconfig(int ch, struct qd_ser_cfg *cfg) { return ser(OP_GETCFG, (unsigned int) ch, (unsigned int) cfg); }
int qd_ser_status(int ch, struct qd_ser_status *st) { return ser(OP_STATUS, (unsigned int) ch, (unsigned int) st); }
int qd_ser_flush(int ch, unsigned int what) { return ser(OP_FLUSH, (unsigned int) ch, what); }
int qd_ser_cancel(int port) { return ser(OP_CANCEL, (unsigned int) port, 0); }
int qd_ser_default_baud(int port, unsigned long baud) { return ser(OP_DEFAULT, (unsigned int) port, (unsigned int) baud); }

/* The kernel wait returns the ready mask at once, or sleeps and returns 0 when woken (data, the deadline, a key, a
 * cancel or a close); the poll after a sleep says which.  The deadline is kept here as well, so a wake for another
 * reason cannot extend the timeout. */
int qd_ser_wait(int ch, unsigned int flags, unsigned long ticks)
{
	unsigned long start = qd_ticks(), left;
	int r;

	flags &= 0x0Fu;
	for (;;) {
		if (ticks == QD_SER_FOREVER)
			left = QD_SER_FOREVER;
		else {
			unsigned long used = qd_ticks() - start;
			left = used >= ticks ? 0 : ticks - used;
			if (left >= QD_SER_FOREVER)
				left = QD_SER_FOREVER - 1;
		}
		r = ser(OP_WAIT, (unsigned int) ch, (flags << 24) | (unsigned int) left);
		if (r != 0)
			return r;                      /* mask, timeout (left == 0), cancelled or an error */
		r = ser(OP_POLL, (unsigned int) ch, flags);
		if (r != 0)
			return r;
		if (ticks != QD_SER_FOREVER && qd_ticks() - start >= ticks)
			return QD_SER_TIMEOUT;
	}
}

int qd_ser_read_timeout(int ch, void *buf, unsigned int n, unsigned long ticks)
{
	unsigned long start = qd_ticks(), used;
	int r;

	for (;;) {
		r = qd_read(ch, buf, n > 256u ? 256u : n);
		if (r != QD_SER_AGAIN)
			return r;
		used = qd_ticks() - start;
		if (ticks != QD_SER_FOREVER && used >= ticks)
			return QD_SER_TIMEOUT;
		r = qd_ser_wait(ch, QD_SER_RX, ticks == QD_SER_FOREVER ? QD_SER_FOREVER : ticks - used);
		if (r < 0)
			return r;
	}
}

int qd_ser_write_all(int ch, const void *buf, unsigned int n, unsigned long ticks)
{
	const unsigned char *p = (const unsigned char *) buf;
	unsigned long start = qd_ticks(), used;
	unsigned int done = 0;
	int r;

	while (done < n) {
		r = qd_write(ch, p + done, n - done > 256u ? 256u : n - done);
		if (r > 0) {
			done += (unsigned int) r;
			continue;
		}
		if (r != QD_SER_AGAIN)
			return r;
		used = qd_ticks() - start;
		if (ticks != QD_SER_FOREVER && used >= ticks)
			return QD_SER_TIMEOUT;
		r = qd_ser_wait(ch, QD_SER_TXSPACE, ticks == QD_SER_FOREVER ? QD_SER_FOREVER : ticks - used);
		if (r < 0)
			return r;
	}
	return (int) n;
}

int qd_ser_drain(int ch, unsigned long ticks)
{
	int r = qd_ser_wait(ch, QD_SER_TXEMPTY, ticks);
	return r > 0 ? 0 : r;
}
