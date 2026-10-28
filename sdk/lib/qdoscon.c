/*
 * qdoscon.c - R11M: SDK wrappers for the virtual text consoles (kernels built with -DQDOS_VCON).  A separate libqdos.a
 * member: a program that does not call these is byte-identical to one linked without it.  Freestanding C89.
 * Rows and columns of the qd_con_* calls are 0-based; qd_con_gotoxy1 is the 1-based form.  See docs/R11M-CONSOLE-ABI.md.
 */
#include "qdos386.h"

#define CC_GOTO   0x10u
#define CC_CLEAR  0x11u
#define CC_ATTR   0x12u
#define CC_QUERY  0x13u
#define CC_MODE   0x14u
#define CC_CURSOR 0x15u
#define CC_REGION 0x16u
#define CC_FOCUS  0x17u
#define CC_CHILD  0x18u
#define CC_SNAP   0x19u
#define CC_DIAG   0x1Au
#define VQ_SIZE   0u
#define VQ_CURSOR 1u
#define VQ_SELF   3u
#define VQ_FG     6u

struct qd_con_snap_req {          /* matches struct chan_con_snap in the kernel (88 bytes) */
	unsigned char con, row, pad[2];
	unsigned char text[84];
};

static int cc(int ch, unsigned int op, unsigned int arg)
{
	return (int) qtrap(QD_TRAP_CH_CTL, (unsigned int) ch, op, arg);
}

int qd_con_goto(int ch, int row, int col)
{
	if (row < 0 || row >= QD_CON_ROWS || col < 0 || col >= QD_CON_COLS)
		return QD_ERR_RANGE;
	return cc(ch, CC_GOTO, (unsigned int) row | ((unsigned int) col << 8));
}

int qd_con_gotoxy1(int ch, int row, int col)
{
	return qd_con_goto(ch, row - 1, col - 1);
}

int qd_con_clear(int ch, int what)
{
	if (what < QD_CLR_ALL || what > QD_CLR_EOS)
		return QD_ERR_RANGE;
	return cc(ch, CC_CLEAR, (unsigned int) what);
}

int qd_con_attr(int ch, int attr)
{
	if (attr < 0 || attr > 255)
		return QD_ERR_RANGE;
	return cc(ch, CC_ATTR, (unsigned int) attr);
}

int qd_con_size(int ch, int *rows, int *cols)
{
	int v = cc(ch, CC_QUERY, VQ_SIZE);

	if (v < 0)
		return v;
	if (cols)
		*cols = v & 0xff;
	if (rows)
		*rows = (v >> 8) & 0xff;
	return 0;
}

int qd_con_cursor(int ch, int *row, int *col)
{
	int v = cc(ch, CC_QUERY, VQ_CURSOR);

	if (v < 0)
		return v;
	if (col)
		*col = v & 0xff;
	if (row)
		*row = (v >> 8) & 0xff;
	return (v >> 16) & 1;          /* 1 if the cursor is visible */
}

int qd_con_show_cursor(int ch, int show)
{
	return cc(ch, CC_CURSOR, show ? 1u : 0u);
}

int qd_con_mode(int ch, unsigned int flags)
{
	if (flags & ~(unsigned int) (QD_CON_ECHO | QD_CON_COOKED))
		return QD_ERR_RANGE;
	return cc(ch, CC_MODE, flags);
}

int qd_con_region(int ch, int top, int bottom)
{
	if (top < 0 || bottom < top || bottom >= QD_CON_ROWS)
		return QD_ERR_RANGE;
	return cc(ch, CC_REGION, (unsigned int) top | ((unsigned int) bottom << 8));
}

int qd_con_number(int ch)
{
	int v = cc(ch, CC_QUERY, VQ_SELF);

	return v < 0 ? v : (v & 0xff);
}

int qd_con_is_foreground(int ch)
{
	int v = cc(ch, CC_QUERY, VQ_SELF);

	return v < 0 ? v : ((v >> 8) & 1);
}

int qd_con_focus(int ch, int con)
{
	if (con < 0 || con >= QD_CON_MAX)
		return QD_ERR_RANGE;
	return cc(ch, CC_FOCUS, (unsigned int) con);
}

int qd_con_child(int ch, int con)
{
	if (con < -1 || con >= QD_CON_MAX)
		return QD_ERR_RANGE;
	return cc(ch, CC_CHILD, con < 0 ? 0xffu : (unsigned int) con);
}

int qd_con_snap(int ch, int con, int row, char *buf, int size)
{
	struct qd_con_snap_req rq;
	int n, i;

	if (!buf || size < 1 || row < 0 || row >= QD_CON_ROWS || con < -1 || con >= QD_CON_MAX)
		return QD_ERR_RANGE;
	rq.con = con < 0 ? 0xffu : (unsigned char) con;
	rq.row = (unsigned char) row;
	rq.pad[0] = rq.pad[1] = 0;
	n = cc(ch, CC_SNAP, (unsigned int) &rq);
	if (n < 0)
		return n;
	for (i = 0; i < n && i < size - 1 && i < (int) sizeof rq.text; i++)
		buf[i] = (char) rq.text[i];
	buf[i] = 0;
	return i;
}

int qd_con_diag(int ch, int mode)
{
	if (mode < -1 || mode > 2)
		return QD_ERR_RANGE;
	return cc(ch, CC_DIAG, mode < 0 ? 0xffu : (unsigned int) mode);
}

/* Blocking one-byte read: the job sleeps in the kernel console wait (no polling) while its console has no input.
 * Returns 0..255, QD_EOF at the end of input, or a negative error.  A second waiter on the same console is refused by
 * the kernel (QD_ERR_CONBUSY); this call then backs off one tick and retries. */
int qd_con_getc(int ch)
{
	volatile unsigned char wake;
	unsigned char c;
	int n;

	for (;;) {
		n = qd_read(ch, &c, 1);
		if (n == 1)
			return (int) c;
		if (n == 0)
			return QD_EOF;
		if (n != -19)
			return n;
		wake = 0;
		if ((int) qtrap(0x1Cu, 0, (unsigned int) &wake, 0) < 0)
			qd_yield(1);
	}
}
