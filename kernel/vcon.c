/*
 * QDOS/386 R11M - virtual text consoles.  See vcon.h and docs/R11M-CONSOLE-ABI.md.
 * Strict C89; empty unless QDOS_VCON is defined.
 */
#include "vcon.h"

#ifdef QDOS_VCON

#define VCON_JOBS 8                    /* job slots tracked (JOB_MAX is 6) */

struct vcon {
	unsigned short cell[VCON_CELLS];
	unsigned int x, y;                 /* cursor; x may be VCON_COLS (wrap pending) */
	unsigned int attr;
	unsigned int top, bot;             /* scroll region, rows, inclusive */
	unsigned int mode;
	int cursor_on;
	int eof;
	unsigned int qh, qt;               /* queue head (write) and tail (read) */
	unsigned char q[VCON_QSIZE];
	unsigned int ll;                   /* cooked line length */
	unsigned char line[VCON_LINE + 1];
	int waiter;                        /* job slot asleep waiting for input, or -1 */
	int users;                         /* live jobs on this console */
	unsigned long dropped;             /* input bytes dropped (queue full) */
};

static struct vcon cons[VCON_MAX];
static int fg = 0;
static int inited = 0;
static int diag_mode = VDIAG_CON0;
static int diag_last = 0;              /* console whose text ended the diagnostic stream */
static int diag_bol = 1;               /* the diagnostic stream is at the start of a line */
static int job_con[VCON_JOBS];
static int job_child[VCON_JOBS];
static int job_live[VCON_JOBS];

static int valid(int con)
{
	return con >= 0 && con < VCON_MAX;
}

static unsigned short mkcell(const struct vcon *v, unsigned int ch)
{
	return (unsigned short) ((ch & 0xffu) | ((v->attr & 0xffu) << 8));
}

static void blank_all(struct vcon *v)
{
	unsigned int i;
	unsigned short b = mkcell(v, ' ');

	for (i = 0; i < VCON_CELLS; i++)
		v->cell[i] = b;
}

static void show_cursor(const struct vcon *v)
{
	unsigned int x = v->x >= VCON_COLS ? VCON_COLS - 1 : v->x;

	if (v == &cons[fg])
		vdisp_cursor(v->y, x, v->cursor_on);
}

void vcon_init(void)
{
	int i;

	for (i = 0; i < VCON_MAX; i++) {
		struct vcon *v = &cons[i];

		v->x = 0;
		v->y = 0;
		v->attr = VCON_ATTR_DEFAULT;
		v->top = 0;
		v->bot = VCON_ROWS - 1;
		v->mode = 0;
		v->cursor_on = 1;
		v->eof = 0;
		v->qh = 0;
		v->qt = 0;
		v->ll = 0;
		v->waiter = -1;
		v->users = 0;
		v->dropped = 0;
		blank_all(v);
	}
	for (i = 0; i < VCON_JOBS; i++) {
		job_con[i] = 0;
		job_child[i] = -1;
		job_live[i] = 0;
	}
	job_live[0] = 1;                   /* SYSTEM, the boot job */
	cons[0].users = 1;
	fg = 0;
	diag_mode = VDIAG_CON0;
	diag_last = 0;
	diag_bol = 1;
	inited = 1;
	vdisp_load(cons[0].cell);
	show_cursor(&cons[0]);
}

static void ensure(void)
{
	if (!inited)
		vcon_init();
}

/* ---- output ---- */

static void put_cell(struct vcon *v, unsigned int idx, unsigned short cell)
{
	v->cell[idx] = cell;
	if (v == &cons[fg])
		vdisp_put(idx / VCON_COLS, idx % VCON_COLS, cell);
}

static void scroll_region(struct vcon *v)
{
	unsigned int r, c;
	unsigned short b = mkcell(v, ' ');

	for (r = v->top; r < v->bot; r++)
		for (c = 0; c < VCON_COLS; c++)
			v->cell[r * VCON_COLS + c] = v->cell[(r + 1) * VCON_COLS + c];
	for (c = 0; c < VCON_COLS; c++)
		v->cell[v->bot * VCON_COLS + c] = b;
	if (v == &cons[fg])
		vdisp_load(v->cell);
}

static void newline(struct vcon *v)
{
	v->x = 0;
	if (v->y == v->bot)
		scroll_region(v);
	else if (v->y < VCON_ROWS - 1)
		v->y++;
}

static void put_byte(struct vcon *v, unsigned char c)
{
	switch (c) {
	case '\r':
		v->x = 0;
		break;
	case '\n':
		newline(v);
		break;
	case '\b':
		if (v->x > 0)
			v->x--;
		break;
	case '\t':
		v->x = (v->x / 8 + 1) * 8;
		if (v->x >= VCON_COLS)
			newline(v);
		break;
	default:
		if (c < 32 || c == 127)
			break;
		if (v->x >= VCON_COLS)
			newline(v);
		put_cell(v, v->y * VCON_COLS + v->x, mkcell(v, c));
		v->x++;
		break;
	}
}

static void diag_out(int con, unsigned char c)
{
	if (diag_mode == VDIAG_NONE)
		return;
	if (diag_mode == VDIAG_CON0 && con != 0)
		return;
	if (diag_mode == VDIAG_ALL) {
		if (con != diag_last) {            /* another console's text: never in the middle of a line */
			if (!diag_bol) {
				diag_putc('\n');
				diag_bol = 1;
			}
			diag_last = con;
		}
		if (con != 0 && diag_bol) {
			diag_putc('[');
			diag_putc('C');
			diag_putc((unsigned char) ('0' + con));
			diag_putc(']');
			diag_putc(' ');
		}
	}
	diag_putc(c);
	diag_bol = (c == '\n');
	diag_last = con;
}

void vcon_write(int con, const unsigned char *buf, unsigned int n)
{
	unsigned int i;
	struct vcon *v;

	ensure();
	if (!valid(con))
		return;
	v = &cons[con];
	for (i = 0; i < n; i++) {
		put_byte(v, buf[i]);
		diag_out(con, buf[i]);
	}
	show_cursor(v);
}

void vcon_kputc(unsigned char c)
{
	struct vcon *v;

	ensure();
	v = &cons[0];
	put_byte(v, c);
	diag_putc(c);                      /* kernel text is diagnostic output in every mode */
	diag_bol = (c == '\n');
	diag_last = 0;
	show_cursor(v);
}

int vcon_goto(int con, unsigned int row, unsigned int col)
{
	struct vcon *v;

	ensure();
	if (!valid(con))
		return VCON_ERR_BADCON;
	v = &cons[con];
	v->y = row >= VCON_ROWS ? VCON_ROWS - 1 : row;
	v->x = col >= VCON_COLS ? VCON_COLS - 1 : col;
	show_cursor(v);
	return 0;
}

void vcon_clear(int con, unsigned int what)
{
	struct vcon *v;
	unsigned int i, from, to, x;
	unsigned short b;

	ensure();
	if (!valid(con))
		return;
	v = &cons[con];
	b = mkcell(v, ' ');
	x = v->x >= VCON_COLS ? VCON_COLS - 1 : v->x;
	switch (what) {
	case VCLR_ALL:
		blank_all(v);
		v->x = 0;
		v->y = 0;
		if (v == &cons[fg])
			vdisp_load(v->cell);
		show_cursor(v);
		return;
	case VCLR_EOL:
		from = v->y * VCON_COLS + x;
		to = (v->y + 1) * VCON_COLS;
		break;
	case VCLR_LINE:
		from = v->y * VCON_COLS;
		to = (v->y + 1) * VCON_COLS;
		break;
	case VCLR_EOS:
		from = v->y * VCON_COLS + x;
		to = VCON_CELLS;
		break;
	default:
		return;
	}
	for (i = from; i < to; i++)
		put_cell(v, i, b);
}

unsigned int vcon_set_attr(int con, unsigned int attr)
{
	unsigned int old;

	ensure();
	if (!valid(con))
		return 0;
	old = cons[con].attr;
	cons[con].attr = attr & 0xffu;
	return old;
}

void vcon_set_cursor_visible(int con, int visible)
{
	ensure();
	if (!valid(con))
		return;
	cons[con].cursor_on = visible != 0;
	show_cursor(&cons[con]);
}

int vcon_set_region(int con, unsigned int top, unsigned int bottom)
{
	ensure();
	if (!valid(con) || top > bottom || bottom >= VCON_ROWS)
		return VCON_ERR_BADCON;
	cons[con].top = top;
	cons[con].bot = bottom;
	return 0;
}

long vcon_query(int con, unsigned int what)
{
	struct vcon *v;
	unsigned int x;

	ensure();
	if (!valid(con))
		return VCON_ERR_BADCON;
	v = &cons[con];
	x = v->x >= VCON_COLS ? VCON_COLS - 1 : v->x;
	switch (what) {
	case VQ_SIZE:
		return (long) (VCON_COLS | (VCON_ROWS << 8));
	case VQ_CURSOR:
		return (long) (x | (v->y << 8) | ((unsigned int) (v->cursor_on ? 1 : 0) << 16));
	case VQ_ATTR:
		return (long) v->attr;
	case VQ_SELF:
		return (long) ((unsigned int) con | ((con == fg ? 1u : 0u) << 8));
	case VQ_MODE:
		return (long) v->mode;
	case VQ_QUEUED:
		return (long) ((v->qh + VCON_QSIZE - v->qt) % VCON_QSIZE);
	case VQ_FG:
		return (long) fg;
	case VQ_REGION:
		return (long) (v->top | (v->bot << 8));
	}
	return VCON_ERR_BADCON;
}

int vcon_snap_row(int con, unsigned int row, unsigned char *out, unsigned int size)
{
	const struct vcon *v;
	unsigned int c, n = 0, last = 0;

	ensure();
	if (!valid(con) || row >= VCON_ROWS || size < 1)
		return VCON_ERR_BADCON;
	v = &cons[con];
	for (c = 0; c < VCON_COLS && n + 1 < size; c++) {
		unsigned int ch = v->cell[row * VCON_COLS + c] & 0xffu;

		if (ch < 32 || ch == 127)
			ch = '.';
		out[n++] = (unsigned char) ch;
		if (ch != ' ')
			last = n;
	}
	out[last] = 0;
	return (int) last;
}

/* ---- focus ---- */

int vcon_switch(int con)
{
	ensure();
	if (!valid(con))
		return VCON_ERR_BADCON;
	if (con == fg)
		return 0;
	fg = con;
	vdisp_load(cons[con].cell);
	show_cursor(&cons[con]);
	return 0;
}

int vcon_fg(void)
{
	return fg;
}

/* ---- input ---- */

static void wake(struct vcon *v)
{
	if (v->waiter >= 0) {
		int w = v->waiter;

		v->waiter = -1;
		vcon_wake_job(w);
	}
}

static void q_put(struct vcon *v, unsigned char c)
{
	unsigned int next = (v->qh + 1) % VCON_QSIZE;

	if (next == v->qt) {
		v->dropped++;
		return;
	}
	v->q[v->qh] = c;
	v->qh = next;
}

static void echo_bytes(int con, const unsigned char *s, unsigned int n)
{
	vcon_write(con, s, n);
}

static void echo_char(int con, unsigned char c)
{
	static const unsigned char crlf[2] = { '\r', '\n' };
	static const unsigned char erase[3] = { '\b', ' ', '\b' };

	if (c == '\r' || c == '\n')
		echo_bytes(con, crlf, 2);
	else if (c == 0x08 || c == 0x7f)
		echo_bytes(con, erase, 3);
	else if (c >= 32 && c < 127)
		echo_bytes(con, &c, 1);
	else if (c == '\t')
		echo_bytes(con, &c, 1);
}

void vcon_input(unsigned char c)
{
	struct vcon *v;

	ensure();
	v = &cons[fg];
	if (v->mode & VC_COOKED) {
		if (c == '\r' || c == '\n') {
			unsigned int free_bytes = (v->qt + VCON_QSIZE - v->qh - 1) % VCON_QSIZE;
			unsigned int i;

			if (v->mode & VC_ECHO)
				echo_char(fg, '\n');
			if (v->ll + 1 <= free_bytes) {
				for (i = 0; i < v->ll; i++)
					q_put(v, v->line[i]);
				q_put(v, '\n');
			} else {
				v->dropped++;
			}
			v->ll = 0;
			wake(v);
		} else if (c == 0x08 || c == 0x7f) {
			if (v->ll > 0) {
				v->ll--;
				if (v->mode & VC_ECHO)
					echo_char(fg, 0x08);
			}
		} else if (c == 0x15) {                 /* ^U */
			while (v->ll > 0) {
				v->ll--;
				if (v->mode & VC_ECHO)
					echo_char(fg, 0x08);
			}
		} else if ((c >= 32 && c < 127) || c == '\t') {
			if (v->ll < VCON_LINE) {
				v->line[v->ll++] = c;
				if (v->mode & VC_ECHO)
					echo_char(fg, c);
			}
		}
		return;
	}
	if (v->mode & VC_ECHO)
		echo_char(fg, c);
	q_put(v, c);
	wake(v);
}

void vcon_eof_all(void)
{
	int i;

	ensure();
	for (i = 0; i < VCON_MAX; i++) {
		cons[i].eof = 1;
		wake(&cons[i]);
	}
}

int vcon_get(int con, unsigned char *c)
{
	struct vcon *v;

	ensure();
	if (!valid(con))
		return 0;
	v = &cons[con];
	if (v->qh == v->qt)
		return 0;
	*c = v->q[v->qt];
	v->qt = (v->qt + 1) % VCON_QSIZE;
	return 1;
}

unsigned int vcon_count(int con)
{
	ensure();
	if (!valid(con))
		return 0;
	return (cons[con].qh + VCON_QSIZE - cons[con].qt) % VCON_QSIZE;
}

int vcon_at_eof(int con)
{
	ensure();
	if (!valid(con))
		return 0;
	return cons[con].eof && cons[con].qh == cons[con].qt;
}

unsigned int vcon_set_mode(int con, unsigned int mode)
{
	unsigned int old;

	ensure();
	if (!valid(con))
		return 0;
	old = cons[con].mode;
	cons[con].mode = mode & VC_MODE_MASK;
	cons[con].ll = 0;                          /* a half-typed line is dropped on a mode change */
	return old;
}

void vcon_set_diag(int mode)
{
	if (mode == VDIAG_CON0 || mode == VDIAG_NONE || mode == VDIAG_ALL)
		diag_mode = mode;
}

int vcon_get_diag(void)
{
	return diag_mode;
}

/* ---- sessions ---- */

int vcon_job_con(int slot)
{
	ensure();
	if (slot < 0 || slot >= VCON_JOBS)
		return 0;
	return job_con[slot];
}

void vcon_job_inherit(int slot, int creator)
{
	int c = 0;

	ensure();
	if (slot < 0 || slot >= VCON_JOBS)
		return;
	if (job_live[slot]) {                      /* defensive: a slot reused without vcon_job_gone */
		cons[job_con[slot]].users--;
		job_live[slot] = 0;
	}
	if (creator >= 0 && creator < VCON_JOBS) {
		c = job_con[creator];
		if (job_child[creator] >= 0) {
			c = job_child[creator];
			job_child[creator] = -1;
		}
	}
	job_con[slot] = c;
	job_child[slot] = -1;
	job_live[slot] = 1;
	cons[c].users++;
}

int vcon_job_set(int slot, int con)
{
	ensure();
	if (slot < 0 || slot >= VCON_JOBS || !valid(con))
		return VCON_ERR_BADCON;
	if (job_live[slot])
		cons[job_con[slot]].users--;
	job_con[slot] = con;
	job_live[slot] = 1;
	cons[con].users++;
	return 0;
}

void vcon_job_gone(int slot)
{
	int i, was;

	ensure();
	if (slot < 0 || slot >= VCON_JOBS)
		return;
	for (i = 0; i < VCON_MAX; i++)
		if (cons[i].waiter == slot)
			cons[i].waiter = -1;
	job_child[slot] = -1;
	was = job_con[slot];
	if (job_live[slot]) {
		job_live[slot] = 0;
		if (cons[was].users > 0)
			cons[was].users--;
		/* the last job of the foreground console ended: the focus returns to console 0 (the shell's) */
		if (was == fg && fg != 0 && cons[fg].users == 0)
			(void) vcon_switch(0);
	}
}

int vcon_set_child(int slot, int con)
{
	ensure();
	if (slot < 0 || slot >= VCON_JOBS || con < -1 || con >= VCON_MAX)
		return VCON_ERR_BADCON;
	job_child[slot] = con;
	return 0;
}

int vcon_waiter(int con)
{
	ensure();
	return valid(con) ? cons[con].waiter : -1;
}

void vcon_set_waiter(int con, int slot)
{
	ensure();
	if (valid(con))
		cons[con].waiter = slot;
}

#endif /* QDOS_VCON */
