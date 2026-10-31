/* QDOS/386 R14 QL compatibility layer: QL CON_/SCR_ windows on the process's 80x25 console session.
 *
 * Geometry: QL pixels are converted to character cells of 6 x 10 pixels; the 80 x 25 cells are taken to be the
 * 480 x 250 pixel area centred in the QL's 512 x 256 screen, so cell (0,0) is pixel (16,3).  A window's origin is
 * rounded to the nearest cell, its size truncated to whole cells; borders take no cells (they are not drawn).
 * sd_wdef of a window that does not fit gives ERR_OR, as a QL does.
 * Text: the QDOS console rules QLTerm relies on - a pending newline after LF and after the last column, sd_donl,
 * cursor movements that fail with ERR_OR at the edges, scrolling and panning by whole cells, clears, ink / strip /
 * paper.  Colours: MODE 4 (black, red, green, white; QL colour & 6) mapped to the CGA attributes; underline is shown as
 * bright ink (VGA colour text has no underline).  sd_fount is accepted and the console's code page 437 font is used
 * (the same IBM graphics glyphs as QLTerm's font).  Keys: native key codes are translated to QL key codes. */
#include <string.h>
#include <qdos386.h>
#include "qlc.h"

#define ROWS 25
#define COLS 80

static unsigned char shadow_c[ROWS][COLS], shadow_a[ROWS][COLS];
static int inited = 0;
static int nat_attr = -1;
static struct qlc_win *cur_owner = 0;   /* the window whose cursor is shown */
static int cur_shown = -1, cur_r = -1, cur_c = -1;

/* a run of cells on one row waiting to be written with one call */
static char run_buf[COLS];
static int run_r = -1, run_c0 = 0, run_n = 0, run_a = 0;

static int cga(int q)
{
	switch (q & 6) {
	case 2: return 4;                    /* red */
	case 4: return 2;                    /* green */
	case 6: return 7;                    /* white */
	}
	return 0;                            /* black */
}

static int attr_of(int ink, int bg, int ul)
{
	return QD_ATTR(cga(ink) | (ul ? 8 : 0), cga(bg));
}

static void set_attr(int a)
{
	if (a != nat_attr) {
		qd_con_attr(qlc_con, a);
		nat_attr = a;
	}
}

static void flush(void)
{
	if (run_n > 0) {
		qd_con_goto(qlc_con, run_r, run_c0);
		set_attr(run_a);
		qd_write(qlc_con, run_buf, (unsigned int) run_n);
		cur_r = -1;                     /* the native cursor moved */
	}
	run_n = 0;
	run_r = -1;
}

static void cell(int r, int c, int ch, int a)
{
	if (r < 0 || r >= ROWS || c < 0 || c >= COLS)
		return;
	shadow_c[r][c] = (unsigned char) ch;
	shadow_a[r][c] = (unsigned char) a;
	if (run_n > 0 && (r != run_r || c != run_c0 + run_n || a != run_a))
		flush();
	if (run_n == 0) {
		run_r = r;
		run_c0 = c;
		run_a = a;
	}
	run_buf[run_n++] = (char) ch;
}

static void redraw_rect(int r0, int c0, int nr, int nc)
{
	int r, c;
	flush();
	for (r = r0; r < r0 + nr; r++)
		for (c = c0; c < c0 + nc; c++)
			cell(r, c, shadow_c[r][c], shadow_a[r][c]);
	flush();
}

static void screen_init(void)
{
	int r, c;
	if (inited)
		return;
	inited = 1;
	qd_con_region(qlc_con, 0, 24);
	for (r = 0; r < ROWS; r++)
		for (c = 0; c < COLS; c++) {
			shadow_c[r][c] = ' ';
			shadow_a[r][c] = (unsigned char) QD_ATTR(7, 0);
		}
	set_attr(QD_ATTR(7, 0));
	qd_con_clear(qlc_con, QD_CLR_ALL);
	qd_con_show_cursor(qlc_con, 0);
	cur_shown = 0;
}

void qlc_win_init(struct qlc_win *w, int is_con)
{
	screen_init();
	memset(w, 0, sizeof *w);
	w->x = 0; w->y = 0; w->w = COLS; w->h = ROWS;
	w->ink = 7; w->paper = 0; w->strip = 0;
	w->is_con = is_con;
}

static void fill(struct qlc_win *w, int r0, int nr, int c0, int nc)
{
	int r, c, a = attr_of(w->ink, w->paper, 0);
	for (r = r0; r < r0 + nr; r++)
		for (c = c0; c < c0 + nc; c++) {
			shadow_c[w->y + r][w->x + c] = ' ';
			shadow_a[w->y + r][w->x + c] = (unsigned char) a;
		}
	redraw_rect(w->y + r0, w->x + c0, nr, nc);
}

/* n > 0: the contents move down n rows; n < 0: up */
static void scroll(struct qlc_win *w, int n)
{
	int r, k = n < 0 ? -n : n;
	if (k >= w->h) {
		fill(w, 0, w->h, 0, w->w);
		return;
	}
	if (n < 0) {
		for (r = 0; r < w->h - k; r++) {
			memcpy(&shadow_c[w->y + r][w->x], &shadow_c[w->y + r + k][w->x], (unsigned int) w->w);
			memcpy(&shadow_a[w->y + r][w->x], &shadow_a[w->y + r + k][w->x], (unsigned int) w->w);
		}
		if (w->w == COLS) {                 /* whole rows: the console scrolls its region itself */
			flush();
			qd_con_region(qlc_con, w->y, w->y + w->h - 1);
			set_attr(attr_of(w->ink, w->paper, 0));
			qd_con_goto(qlc_con, w->y + w->h - 1, 0);
			for (r = 0; r < k; r++)
				qd_write(qlc_con, "\n", 1);
			qd_con_region(qlc_con, 0, 24);
			cur_r = -1;
			for (r = w->h - k; r < w->h; r++)
				for (n = 0; n < w->w; n++) {
					shadow_c[w->y + r][w->x + n] = ' ';
					shadow_a[w->y + r][w->x + n] = (unsigned char) attr_of(w->ink, w->paper, 0);
				}
			return;
		}
		fill(w, w->h - k, k, 0, w->w);
		redraw_rect(w->y, w->x, w->h - k, w->w);
	} else {
		for (r = w->h - 1; r >= k; r--) {
			memcpy(&shadow_c[w->y + r][w->x], &shadow_c[w->y + r - k][w->x], (unsigned int) w->w);
			memcpy(&shadow_a[w->y + r][w->x], &shadow_a[w->y + r - k][w->x], (unsigned int) w->w);
		}
		fill(w, 0, k, 0, w->w);
		redraw_rect(w->y + k, w->x, w->h - k, w->w);
	}
}

static void pan(struct qlc_win *w, int n)       /* n > 0: right */
{
	int r, k = n < 0 ? -n : n;
	if (k >= w->w) {
		fill(w, 0, w->h, 0, w->w);
		return;
	}
	for (r = 0; r < w->h; r++) {
		unsigned char *pc = &shadow_c[w->y + r][w->x], *pa = &shadow_a[w->y + r][w->x];
		if (n > 0) {
			memmove(pc + k, pc, (unsigned int) (w->w - k));
			memmove(pa + k, pa, (unsigned int) (w->w - k));
		} else {
			memmove(pc, pc + k, (unsigned int) (w->w - k));
			memmove(pa, pa + k, (unsigned int) (w->w - k));
		}
	}
	if (n > 0)
		fill(w, 0, w->h, 0, k);
	else
		fill(w, 0, w->h, w->w - k, k);
	redraw_rect(w->y, w->x, w->h, w->w);
}

static void newline(struct qlc_win *w)
{
	w->cx = 0;
	if (w->cy < w->h - 1)
		w->cy++;
	else
		scroll(w, -1);
}

int qlc_win_putc(struct qlc_win *w, int c)
{
	if (c == 10) {
		if (w->pend)
			newline(w);
		w->pend = 1;
		return 0;
	}
	if (c < 32 && c != 9)
		return 0;                         /* QLTerm handles the other control codes itself */
	if (w->pend) {
		newline(w);
		w->pend = 0;
	}
	if (c == 9)
		c = ' ';
	cell(w->y + w->cy, w->x + w->cx, c, attr_of(w->ink, w->strip, w->ul));
	if (w->cx + 1 >= w->w)
		w->pend = 1;                     /* the next character starts a new line */
	else
		w->cx++;
	return 0;
}

void qlc_win_cursor_sync(void)
{
	struct qlc_win *w = cur_owner;
	int show = w != 0 && w->cursor_on, r = 0, c = 0;
	flush();
	if (show) {
		r = w->y + w->cy;
		c = w->x + w->cx;
		if (r != cur_r || c != cur_c) {
			qd_con_goto(qlc_con, r, c);
			cur_r = r;
			cur_c = c;
		}
	}
	if (show != cur_shown) {
		qd_con_show_cursor(qlc_con, show);
		cur_shown = show;
	}
}

/* ---- sd_* ---- */

static struct qlc_win *win(chanid_t id)
{
	struct qlc_chan *c = qlc_get(id);
	return (c && c->kind == QLC_K_WIN) ? &c->win : 0;
}

#define WIN_OR_ERR(w, id) struct qlc_win *w = win(id); (void) t; if (w == 0) return qlc_get(id) ? ERR_BP : ERR_NO

static int done(int r) { qlc_win_cursor_sync(); return r; }

int sd_cure(chanid_t id, timeout_t t) { WIN_OR_ERR(w, id); w->cursor_on = 1; cur_owner = w; return done(0); }
int sd_curs(chanid_t id, timeout_t t) { WIN_OR_ERR(w, id); w->cursor_on = 0; return done(0); }

int sd_pos(chanid_t id, timeout_t t, short col, short row)
{
	WIN_OR_ERR(w, id);
	if (col < 0 || col >= w->w || row < 0 || row >= w->h)
		return done(ERR_OR);
	w->cx = col; w->cy = row; w->pend = 0;
	return done(0);
}

int sd_tab(chanid_t id, timeout_t t, short col)
{
	WIN_OR_ERR(w, id);
	if (col < 0 || col >= w->w)
		return done(ERR_OR);
	w->cx = col; w->pend = 0;
	return done(0);
}

int sd_ncol(chanid_t id, timeout_t t) { WIN_OR_ERR(w, id); if (w->cx + 1 >= w->w) return done(ERR_OR); w->cx++; w->pend = 0; return done(0); }
int sd_pcol(chanid_t id, timeout_t t) { WIN_OR_ERR(w, id); if (w->cx - 1 < 0) return done(ERR_OR); w->cx--; w->pend = 0; return done(0); }
int sd_nrow(chanid_t id, timeout_t t) { WIN_OR_ERR(w, id); if (w->cy + 1 >= w->h) return done(ERR_OR); w->cy++; w->pend = 0; return done(0); }
int sd_prow(chanid_t id, timeout_t t) { WIN_OR_ERR(w, id); if (w->cy - 1 < 0) return done(ERR_OR); w->cy--; w->pend = 0; return done(0); }
int sd_donl(chanid_t id, timeout_t t) { WIN_OR_ERR(w, id); if (w->pend) { newline(w); w->pend = 0; } return done(0); }

int sd_scrol(chanid_t id, timeout_t t, short px)
{
	WIN_OR_ERR(w, id);
	if (px / 10)
		scroll(w, px / 10);
	return done(0);
}

int sd_pan(chanid_t id, timeout_t t, short px)
{
	WIN_OR_ERR(w, id);
	if (px / 6)
		pan(w, px / 6);
	return done(0);
}

int sd_clear(chanid_t id, timeout_t t) { WIN_OR_ERR(w, id); fill(w, 0, w->h, 0, w->w); w->cx = w->cy = 0; w->pend = 0; return done(0); }
int sd_clrtp(chanid_t id, timeout_t t) { WIN_OR_ERR(w, id); if (w->cy > 0) fill(w, 0, w->cy, 0, w->w); return done(0); }
int sd_clrbt(chanid_t id, timeout_t t) { WIN_OR_ERR(w, id); if (w->cy + 1 < w->h) fill(w, w->cy + 1, w->h - w->cy - 1, 0, w->w); return done(0); }
int sd_clrln(chanid_t id, timeout_t t) { WIN_OR_ERR(w, id); fill(w, w->cy, 1, 0, w->w); return done(0); }
int sd_clrrt(chanid_t id, timeout_t t) { WIN_OR_ERR(w, id); fill(w, w->cy, 1, w->cx, w->w - w->cx); return done(0); }
int sd_setul(chanid_t id, timeout_t t, int on) { WIN_OR_ERR(w, id); w->ul = on != 0; return 0; }
int _sd_setin(chanid_t id, timeout_t t, short c) { WIN_OR_ERR(w, id); w->ink = c & 7; return 0; }
int _sd_setst(chanid_t id, timeout_t t, short c) { WIN_OR_ERR(w, id); w->strip = c & 7; return 0; }
int _sd_setpa(chanid_t id, timeout_t t, short c) { WIN_OR_ERR(w, id); w->paper = c & 7; return 0; }
int sd_fount(chanid_t id, timeout_t t, char *f1, char *f2) { WIN_OR_ERR(w, id); (void) f1; (void) f2; return 0; }

int sd_chenq(chanid_t id, timeout_t t, QLRECT_t *r)
{
	WIN_OR_ERR(w, id);
	r->q_width = (short) w->w; r->q_height = (short) w->h;
	r->q_x = (short) w->cx; r->q_y = (short) w->cy;
	return 0;
}

static int cells_floor(int px, int unit) { return px >= 0 ? px / unit : -((-px + unit - 1) / unit); }

int _sd_wdef(chanid_t id, timeout_t t, short bc, short bw, QLRECT_t *rc)
{
	int x0, y0, w0, h0, c, r, nc, nr;
	WIN_OR_ERR(w, id);
	(void) bc;
	if (bw < 0)
		return ERR_BP;
	x0 = rc->q_x + 2 * bw; y0 = rc->q_y + bw;
	w0 = rc->q_width - 4 * bw; h0 = rc->q_height - 2 * bw;
	c = cells_floor(x0 - 16 + 3, 6);
	r = cells_floor(y0 - 3 + 5, 10);
	nc = w0 / 6; nr = h0 / 10;
	if (nc < 1 || nr < 1 || c < 0 || r < 0 || c + nc > COLS || r + nr > ROWS)
		return ERR_OR;
	w->x = c; w->y = r; w->w = nc; w->h = nr;
	w->cx = w->cy = 0; w->pend = 0;
	return 0;
}

/* ---- keys ---- */

int qlc_key(int k)
{
	static const unsigned char fk[10] = { 0xE8, 0xEC, 0xF0, 0xF4, 0xF8, 0xEA, 0xEE, 0xF2, 0xF6, 0xFA };
	if (k == 0x0D || k == 0x0A) return 10;          /* ENTER */
	if (k == 0x08 || k == 0x7F) return 0xC2;        /* backspace: CTRL+LEFT (delete left) */
	if (k >= 0 && k < 0x7F) return k;               /* text, ESC, TAB, CTRL+letters */
	switch (k) {
	case QD_KEY_UP: return 0xD0;
	case QD_KEY_DOWN: return 0xD8;
	case QD_KEY_LEFT: return 0xC0;
	case QD_KEY_RIGHT: return 0xC8;
	case QD_KEY_HOME: return 0xC1;                  /* ALT+LEFT */
	case QD_KEY_END: return 0xC9;                   /* ALT+RIGHT */
	case QD_KEY_PGUP: return 0xD4;                  /* SHIFT+UP */
	case QD_KEY_PGDN: return 0xDC;                  /* SHIFT+DOWN */
	case QD_KEY_DEL: return 0xCA;                   /* CTRL+RIGHT (delete right) */
	}
	if (k >= QD_KEY_F1 && k <= QD_KEY_F1 + 9)
		return fk[k - QD_KEY_F1];                   /* F1..F5, F6..F10 = SHIFT+F1..F5 */
	return -1;
}

/* ---- IO.EDLIN: edit the line that ends at *p (length *len, cursor cur); ENTER, ESC, UP, DOWN end it and are
 * stored after the text; *p then points after the terminator and *len counts it.  Left/right move, CTRL+LEFT
 * (backspace) and CTRL+RIGHT (delete) delete.  Text beyond the window's right edge is kept but not shown. ---- */
int io_edlin(chanid_t id, timeout_t t, char **p, short max, short cur, short *len)
{
	struct qlc_win *w = win(id);
	char *b, kc;
	int n, pos, x0, i, r, k;
	if (w == 0)
		return qlc_get(id) ? ERR_BP : ERR_NO;
	n = *len;
	if (n < 0 || n > max || cur < 0 || cur > n)
		return ERR_BP;
	b = *p - n;
	pos = cur;
	x0 = w->cx - pos;
	if (x0 < 0)
		x0 = 0;
	for (;;) {
		/* show the line, then the cursor */
		w->pend = 0;
		w->cx = x0;
		for (i = 0; i <= n && x0 + i < w->w; i++)
			cell(w->y + w->cy, w->x + x0 + i, i < n ? (unsigned char) b[i] : ' ', attr_of(w->ink, w->strip, w->ul));
		w->cx = x0 + pos < w->w ? x0 + pos : w->w - 1;
		qlc_win_cursor_sync();
		r = io_fbyte(id, t, &kc);
		if (r != 0)
			return r;
		k = (unsigned char) kc;
		if (k == 10 || k == 27 || k == 0xD0 || k == 0xD8) {
			if (n >= max)
				return ERR_BO;
			b[n++] = (char) k;
			*p = b + n;
			*len = (short) n;
			w->cx = x0 + n - 1 < w->w ? x0 + n - 1 : w->w - 1;
			return 0;
		}
		if (k == 0xC0) { if (pos > 0) pos--; continue; }
		if (k == 0xC8) { if (pos < n) pos++; continue; }
		if (k == 0xC2) {
			if (pos > 0) { memmove(b + pos - 1, b + pos, (unsigned int) (n - pos)); pos--; n--; }
			continue;
		}
		if (k == 0xCA) {
			if (pos < n) { memmove(b + pos, b + pos + 1, (unsigned int) (n - pos - 1)); n--; }
			continue;
		}
		if (k >= 32 && k < 0x7F && n < max - 1) {
			memmove(b + pos + 1, b + pos, (unsigned int) (n - pos));
			b[pos++] = (char) k;
			n++;
		}
	}
}
