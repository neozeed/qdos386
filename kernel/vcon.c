/*
 * QDOS/386 R11M - virtual text consoles.  See vcon.h and docs/R11M-CONSOLE-ABI.md.
 * Strict C89; empty unless QDOS_VCON is defined.
 */
#include "vcon.h"

#ifdef QDOS_VCON
#ifdef QDOS_SURFACES
#include "present.h"
#include "memory.h"
#include "jobs.h"
#include "channel.h"
#endif

#ifdef QDOS_DYNPROC
#define VCON_JOBS 16                   /* R12A: JOB_MAX is 16 */
#else
#define VCON_JOBS 8                    /* job slots tracked (JOB_MAX is 6) */
#endif

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
#ifdef QDOS_SURFACES
	struct gsurf *g;                   /* R13: the INDEX8 surface shown instead of the cells, or 0 */
	unsigned int tag;                  /* R13: changes each time the index is reused */
	unsigned int owner;                /* R13: tagged id of the creator of a private session */
#endif
};

static struct vcon cons[VCON_MAX];
#ifdef QDOS_SURFACES
/* R13: the session table.  Sessions 0..3 are the legacy consoles (static, Alt+F1..F4); 4..VSESS_MAX-1 are private
 * sessions allocated from the kernel heap on demand and freed when their last job ends. */
static struct vcon *ctab[VSESS_MAX];
#define CON(i) (*ctab[i])
#else
#define CON(i) cons[i]
#endif
static int fg = 0;
#ifdef QDOS_TASKMGR
/* R12C: while the task selector overlay owns the display, console output only updates the cell buffers; the
 * foreground console is drawn again when the overlay closes (vcon_overlay). */
static int overlay = 0;
#define SHOWN(v) (!overlay && (v) == &CON(fg))
#else
#define SHOWN(v) ((v) == &CON(fg))
#endif
static int inited = 0;
static int diag_mode = VDIAG_CON0;
static int diag_last = 0;              /* console whose text ended the diagnostic stream */
static int diag_bol = 1;               /* the diagnostic stream is at the start of a line */
static int job_con[VCON_JOBS];
static int job_child[VCON_JOBS];
static int job_live[VCON_JOBS];

#ifdef QDOS_SURFACES
static unsigned int stag_next = 1;
static int new_failures;
static void sess_free(int idx);
static int valid(int con)
{
	return con >= 0 && con < VSESS_MAX && ctab[con] != 0;
}
#else
static int valid(int con)
{
	return con >= 0 && con < VCON_MAX;
}
#endif

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

	if (SHOWN(v))
		vdisp_cursor(v->y, x, v->cursor_on);
}

#ifdef QDOS_SURFACES
static void sess_reset(struct vcon *v)
{
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
	v->g = 0;
	v->owner = 0;
	v->tag = stag_next++;
	if (stag_next > 0xffffffu)
		stag_next = 1;
	blank_all(v);
}
#endif

void vcon_init(void)
{
	int i;

#ifdef QDOS_SURFACES
	for (i = 0; i < VSESS_MAX; i++)
		ctab[i] = i < VCON_MAX ? &cons[i] : 0;
#endif
	for (i = 0; i < VCON_MAX; i++) {
		struct vcon *v = &CON(i);

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
#ifdef QDOS_SURFACES
		sess_reset(v);
#endif
	}
	for (i = 0; i < VCON_JOBS; i++) {
		job_con[i] = 0;
		job_child[i] = -1;
		job_live[i] = 0;
	}
	job_live[0] = 1;                   /* SYSTEM, the boot job */
	CON(0).users = 1;
	fg = 0;
	diag_mode = VDIAG_CON0;
	diag_last = 0;
	diag_bol = 1;
	inited = 1;
	vdisp_load(CON(0).cell);
	show_cursor(&CON(0));
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
	if (SHOWN(v))
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
	if (SHOWN(v))
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
#ifdef QDOS_SURFACES
			if (con >= 10)
				diag_putc((unsigned char) ('0' + con / 10));
			diag_putc((unsigned char) ('0' + con % 10));
#else
			diag_putc((unsigned char) ('0' + con));
#endif
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
	v = &CON(con);
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
	v = &CON(0);
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
	v = &CON(con);
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
	v = &CON(con);
	b = mkcell(v, ' ');
	x = v->x >= VCON_COLS ? VCON_COLS - 1 : v->x;
	switch (what) {
	case VCLR_ALL:
		blank_all(v);
		v->x = 0;
		v->y = 0;
		if (SHOWN(v))
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
	old = CON(con).attr;
	CON(con).attr = attr & 0xffu;
	return old;
}

void vcon_set_cursor_visible(int con, int visible)
{
	ensure();
	if (!valid(con))
		return;
	CON(con).cursor_on = visible != 0;
	show_cursor(&CON(con));
}

int vcon_set_region(int con, unsigned int top, unsigned int bottom)
{
	ensure();
	if (!valid(con) || top > bottom || bottom >= VCON_ROWS)
		return VCON_ERR_BADCON;
	CON(con).top = top;
	CON(con).bot = bottom;
	return 0;
}

long vcon_query(int con, unsigned int what)
{
	struct vcon *v;
	unsigned int x;

	ensure();
	if (!valid(con))
		return VCON_ERR_BADCON;
	v = &CON(con);
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
	v = &CON(con);
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
#ifdef QDOS_TASKMGR
	if (overlay)
		return 0;                      /* drawn when the overlay closes */
#endif
#ifdef QDOS_SURFACES
	pres_focus_changed();              /* R13: the presenter shows a graphics session, or leaves graphics mode */
	if (CON(con).g != 0 || !pres_text_ok())
		return 0;                      /* drawn by the presenter (SYSTEM job), never here in the keyboard IRQ */
#endif
	vdisp_load(CON(con).cell);
	show_cursor(&CON(con));
	return 0;
}

#ifdef QDOS_TASKMGR
/* R12C: on = the caller (the task selector) owns the display from now on; off = draw the foreground console and its
 * cursor again.  No console buffer is changed either way. */
void vcon_overlay(int on)
{
	ensure();
	if (on) {
		overlay = 1;
#ifdef QDOS_SURFACES
		pres_focus_changed();          /* the selector needs text mode */
#endif
		return;
	}
	if (!overlay)
		return;
	overlay = 0;
#ifdef QDOS_SURFACES
	pres_focus_changed();
	if (CON(fg).g != 0 || !pres_text_ok())
		return;
#endif
	vdisp_load(CON(fg).cell);
	vdisp_cursor(CON(fg).y, CON(fg).x >= VCON_COLS ? VCON_COLS - 1 : CON(fg).x, CON(fg).cursor_on);
}

int vcon_users(int con)
{
	ensure();
	return valid(con) ? CON(con).users : 0;
}
#endif

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
#ifdef QDOS_TASKMGR
	if (taskmgr_key(c))                /* R12C: the open task selector takes every key; nothing reaches a program */
		return;
#endif
	v = &CON(fg);
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
#ifdef QDOS_SURFACES
	for (i = 0; i < VSESS_MAX; i++) {
		if (!valid(i))
			continue;
		CON(i).eof = 1;
		wake(&CON(i));
	}
#else
	for (i = 0; i < VCON_MAX; i++) {
		CON(i).eof = 1;
		wake(&CON(i));
	}
#endif
}

int vcon_get(int con, unsigned char *c)
{
	struct vcon *v;

	ensure();
	if (!valid(con))
		return 0;
	v = &CON(con);
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
	return (CON(con).qh + VCON_QSIZE - CON(con).qt) % VCON_QSIZE;
}

int vcon_at_eof(int con)
{
	ensure();
	if (!valid(con))
		return 0;
	return CON(con).eof && CON(con).qh == CON(con).qt;
}

unsigned int vcon_set_mode(int con, unsigned int mode)
{
	unsigned int old;

	ensure();
	if (!valid(con))
		return 0;
	old = CON(con).mode;
	CON(con).mode = mode & VC_MODE_MASK;
	CON(con).ll = 0;                          /* a half-typed line is dropped on a mode change */
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
		CON(job_con[slot]).users--;
		job_live[slot] = 0;
	}
	if (creator >= 0 && creator < VCON_JOBS) {
		c = job_con[creator];
#ifdef QDOS_SURFACES
		if (job_child[creator] == VCON_CHILD_NEW) {     /* R13: a private session for the new job */
			int n = vsess_new(jobs[slot].id);

			job_child[creator] = -1;
			if (n >= 0)
				c = n;
			else
				new_failures++;                     /* the creator's session: reported by vsess_new_failures */
		} else if (job_child[creator] >= 0 && !valid(job_child[creator])) {
			job_child[creator] = -1;                    /* the chosen session has ended meanwhile */
		}
#endif
		if (job_child[creator] >= 0) {
			c = job_child[creator];
			job_child[creator] = -1;
		}
	}
	job_con[slot] = c;
	job_child[slot] = -1;
	job_live[slot] = 1;
	CON(c).users++;
}

int vcon_job_set(int slot, int con)
{
	ensure();
	if (slot < 0 || slot >= VCON_JOBS || !valid(con))
		return VCON_ERR_BADCON;
	if (job_live[slot])
		CON(job_con[slot]).users--;
	job_con[slot] = con;
	job_live[slot] = 1;
	CON(con).users++;
	return 0;
}

void vcon_job_gone(int slot)
{
	int i, was;

	ensure();
	if (slot < 0 || slot >= VCON_JOBS)
		return;
#ifdef QDOS_SURFACES
	for (i = 0; i < VSESS_MAX; i++)
		if (valid(i) && CON(i).waiter == slot)
			CON(i).waiter = -1;
	job_child[slot] = -1;
	was = job_con[slot];
	if (job_live[slot]) {
		job_live[slot] = 0;
		surf_job_ended(jobs[slot].id, was);         /* its graphics surface, if it opened one */
		if (CON(was).users > 0)
			CON(was).users--;
		if (CON(was).users == 0 && was >= VCON_MAX)
			sess_free(was);                         /* a private session ends with its last job */
		else if (was == fg && fg != 0 && CON(fg).users == 0)
			(void) vcon_switch(0);
	}
#else
	for (i = 0; i < VCON_MAX; i++)
		if (CON(i).waiter == slot)
			CON(i).waiter = -1;
	job_child[slot] = -1;
	was = job_con[slot];
	if (job_live[slot]) {
		job_live[slot] = 0;
		if (CON(was).users > 0)
			CON(was).users--;
		/* the last job of the foreground console ended: the focus returns to console 0 (the shell's) */
		if (was == fg && fg != 0 && CON(fg).users == 0)
			(void) vcon_switch(0);
	}
#endif
}

int vcon_set_child(int slot, int con)
{
	ensure();
#ifdef QDOS_SURFACES
	if (slot < 0 || slot >= VCON_JOBS || (con != -1 && con != VCON_CHILD_NEW && !valid(con)))
		return VCON_ERR_BADCON;
#else
	if (slot < 0 || slot >= VCON_JOBS || con < -1 || con >= VCON_MAX)
		return VCON_ERR_BADCON;
#endif
	job_child[slot] = con;
	return 0;
}

int vcon_waiter(int con)
{
	ensure();
	return valid(con) ? CON(con).waiter : -1;
}

void vcon_set_waiter(int con, int slot)
{
	ensure();
	if (valid(con))
		CON(con).waiter = slot;
}

#ifdef QDOS_SURFACES
/* ---- R13: private sessions ---- */

int vsess_new(unsigned int owner_id)
{
	int i;
	struct vcon *v;

	ensure();
	for (i = VCON_MAX; i < VSESS_MAX; i++)
		if (ctab[i] == 0)
			break;
	if (i >= VSESS_MAX)
		return VSESS_ERR_FULL;
	v = (struct vcon *) mem_alloc((unsigned int) sizeof(struct vcon), VSESS_OWNER);
	if (mem_is_error((unsigned int) v))
		return VSESS_ERR_MEM;
	sess_reset(v);
	v->owner = owner_id;
	ctab[i] = v;
	return i;
}

static void sess_free(int idx)
{
	struct vcon *v;
	int s;

	if (idx < VCON_MAX || !valid(idx))
		return;
	v = ctab[idx];
	if (v->g != 0) {
		struct gsurf *g = v->g;

		v->g = 0;
		surf_release(g);
	}
	for (s = 0; s < VCON_JOBS; s++)
		if (job_child[s] == idx)
			job_child[s] = -1;
	ctab[idx] = 0;
	(void) mem_free(v, VSESS_OWNER);
	if (fg == idx)
		(void) vcon_switch(0);                     /* the shown session ended: back to console 0 */
}

int vsess_valid(int idx)
{
	ensure();
	return valid(idx);
}

unsigned int vsess_tag(int idx)
{
	return valid(idx) ? CON(idx).tag : 0u;
}

int vsess_move(int slot, int idx)
{
	int old;

	ensure();
	if (slot < 0 || slot >= VCON_JOBS || !valid(idx) || !job_live[slot])
		return VCON_ERR_BADCON;
	old = job_con[slot];
	if (old == idx)
		return 0;
	if (CON(old).waiter == slot)
		CON(old).waiter = -1;
	job_con[slot] = idx;
	CON(idx).users++;
	chan_con_retarget(jobs[slot].id, old, idx);
	if (CON(old).users > 0)
		CON(old).users--;
	if (CON(old).users == 0 && old >= VCON_MAX)
		sess_free(old);
	return 0;
}

struct gsurf *vsess_gfx(int idx)
{
	return valid(idx) ? CON(idx).g : 0;
}

void vsess_set_gfx(int idx, struct gsurf *g)
{
	if (!valid(idx))
		return;
	CON(idx).g = g;
	CON(idx).tag = stag_next++;                    /* a mode change is a new surface for stale-id checks */
	if (stag_next > 0xffffffu)
		stag_next = 1;
	if (idx == fg)
		pres_focus_changed();
}

int vsess_users(int idx)
{
	return valid(idx) ? CON(idx).users : 0;
}

int vsess_dynamic(int idx)
{
	return valid(idx) && idx >= VCON_MAX;
}

unsigned int vsess_owner(int idx)
{
	return valid(idx) ? CON(idx).owner : 0u;
}

int vsess_count(void)
{
	int i, n = 0;

	for (i = 0; i < VSESS_MAX; i++)
		if (valid(i))
			n++;
	return n;
}

int vsess_new_failures(void)
{
	return new_failures;
}

int vcon_overlay_on(void)
{
#ifdef QDOS_TASKMGR
	return overlay;
#else
	return 0;
#endif
}

/* The presenter has put the display back into text mode: draw the foreground console (the open selector redraws
 * itself on its next step). */
void vcon_present_text(void)
{
	if (vcon_overlay_on() || CON(fg).g != 0)
		return;
	vdisp_load(CON(fg).cell);
	vdisp_cursor(CON(fg).y, CON(fg).x >= VCON_COLS ? VCON_COLS - 1 : CON(fg).x, CON(fg).cursor_on);
}

int vcon_write_cells(int con, unsigned int row, unsigned int col, const unsigned short *cells, unsigned int n)
{
	struct vcon *v;
	unsigned int i, idx;

	ensure();
	if (!valid(con) || row >= VCON_ROWS || col >= VCON_COLS || n > VCON_CELLS - (row * VCON_COLS + col))
		return VCON_ERR_BADCON;
	v = &CON(con);
	idx = row * VCON_COLS + col;
	for (i = 0; i < n; i++)
		put_cell(v, idx + i, cells[i]);
	return (int) n;
}
#endif /* QDOS_SURFACES */

#endif /* QDOS_VCON */
