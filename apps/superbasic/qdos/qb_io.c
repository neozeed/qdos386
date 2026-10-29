/*
 * qb_io.c - SuperBASIC channel adapters and the console line editor.
 *
 * Channels (see docs/R12-SUPERBASIC-PORT.md):
 *   #0 #1 #2   read/write   the job's CON channel (output: qd_write, input: lines)
 *   #3         write only   the same console
 *   diag       write only   the same console, bright red (error reports)
 * None of them owns an operating-system channel: the console channel is the
 * job's own and is closed by the system at exit.
 *
 * Input.  The console is put in raw mode (no echo, keys arrive as single
 * bytes, cursor and function keys as codes 0x80..0x99).  qb_getline() is a
 * small line editor drawn with absolute cursor moves from the cell where the
 * line starts (the same method as the shell's editor), so a line may wrap
 * over several rows.  Lines are kept in a history (Up/Down).  The read
 * adapter hands a typed line to the core byte by byte; an INPUT statement
 * therefore gets an editor and a history too.
 *
 * Break.  While a program writes to the console, typed keys are looked at
 * (qd_con_queued): Ctrl+C ends the program with the ROM's "not complete"
 * error; every other key is kept in a small type-ahead queue for the next
 * line read.  A program that computes without printing is not interrupted
 * (the shell's KILL /CON:n ends it).
 *
 * A kernel without the R12 console API (no key codes) gets cooked-mode line
 * input from the kernel instead of the editor.
 */
#include <stdlib.h>
#include <string.h>
#include "qdos386.h"
#include "sb_chan.h"
#include "sb_error.h"
#include "sb_interp.h"
#include "sb_state.h"
#include "qb.h"

int qd_con_channel(void);

#define K_BS      0x08
#define K_DBS     0x7F
#define K_ENTER   0x0D
#define K_LF      0x0A
#define K_CTRL_A  0x01
#define K_CTRL_B  0x02
#define K_CTRL_C  0x03
#define K_CTRL_D  0x04
#define K_CTRL_E  0x05
#define K_CTRL_F  0x06
#define K_CTRL_K  0x0B
#define K_CTRL_N  0x0E
#define K_CTRL_P  0x10
#define K_CTRL_U  0x15

#define OUT_CHUNK 200

static int con = -1;                 /* the job's CON channel */
static int editor = 0;
static int old_mode = 0;
static int mode_saved = 0;

static unsigned char tah[64];        /* type-ahead (keys seen while a program printed) */
static int tah_n, tah_r;
static int break_flag;

static char line[QB_LINE_MAX + 2];
static unsigned int len, pos;
static int sr, sc;                   /* the cell where line[0] is drawn */
static char hist[QB_HIST_N][QB_LINE_MAX + 1];
static int hist_n, hist_i;
static char stash[QB_LINE_MAX + 1];

static char pend[QB_LINE_MAX + 2];   /* the typed line being handed to the core */
static int pend_len, pend_pos;

/* ---- output ---------------------------------------------------------- */

void qb_out(const char *s, unsigned int n)
{
	while (n > 0u) {
		unsigned int k = n > OUT_CHUNK ? OUT_CHUNK : n;
		int w = qd_write(con, s, k);

		if (w <= 0)
			return;
		s += w;
		n -= (unsigned int) w;
	}
}

void qb_puts(const char *s)
{
	qb_out(s, (unsigned int) strlen(s));
}

static void outc(int c)
{
	char ch = (char) c;

	qb_out(&ch, 1u);
}

/* ---- keys ------------------------------------------------------------ */

static int next_key(void)
{
	if (tah_r < tah_n)
		return tah[tah_r++];
	tah_n = tah_r = 0;
	return qd_con_getkey(con);
}

/* Look at the keys typed while the program ran: Ctrl+C sets the break flag,
 * the rest goes to the type-ahead queue (dropped when it is full). */
static void poll_keys(void)
{
	int guard = 0;

	while (guard++ < 128 && qd_con_queued(con) > 0) {
		int k = qd_con_getkey(con);

		if (k < 0)
			return;
		if (k == K_CTRL_C) {
			break_flag = 1;
			continue;
		}
		if (tah_r == tah_n)
			tah_r = tah_n = 0;
		if (tah_n < (int) sizeof(tah))
			tah[tah_n++] = (unsigned char) k;
	}
}

int qb_break_seen(void)
{
	int f = break_flag;

	break_flag = 0;
	return f;
}

/* ---- the line editor ------------------------------------------------- */

static void ed_goto(int r, int c)
{
	qd_con_goto(con, r, c);
}

/* Make the cell after the last character of a line of n characters fit on
 * the screen by scrolling up first.  Returns 1 if it scrolled (redraw all). */
static int ed_fit(unsigned int n)
{
	int last = sr + (int) ((unsigned int) sc + n) / 80;
	int d = last - 24, k;

	if (d <= 0)
		return 0;
	if (d > sr)
		d = sr;
	ed_goto(24, 0);
	for (k = 0; k < d; k++)
		outc('\n');
	sr -= d;
	return 1;
}

static void ed_cursor(void)
{
	unsigned int off = (unsigned int) sc + pos;

	ed_goto(sr + (int) (off / 80), (int) (off % 80));
}

/* Redraw line[from..], erase what followed, put the cursor at pos. */
static void ed_draw(unsigned int from)
{
	unsigned int off;

	if (ed_fit(len))
		from = 0;
	off = (unsigned int) sc + from;
	ed_goto(sr + (int) (off / 80), (int) (off % 80));
	if (from < len)
		qb_out(line + from, len - from);
	/* clear from the cell AFTER the last character: the console cursor may
	 * be in the pending-wrap state, where "clear from the cursor" would
	 * erase the character just drawn in the last column */
	off = (unsigned int) sc + len;
	ed_goto(sr + (int) (off / 80), (int) (off % 80));
	qd_con_clear(con, QD_CLR_EOS);
	ed_cursor();
}

static void ed_load(const char *s)
{
	unsigned int n = 0;

	while (s[n] && n < QB_LINE_MAX) {
		line[n] = s[n];
		n++;
	}
	line[n] = 0;
	len = pos = n;
	ed_draw(0);
}

static void ed_insert(int c)
{
	unsigned int i;
	int scrolled;

	if (len >= QB_LINE_MAX)
		return;
	scrolled = ed_fit(len + 1);
	for (i = len; i > pos; i--)
		line[i] = line[i - 1];
	line[pos] = (char) c;
	len++;
	pos++;
	line[len] = 0;
	if (!scrolled && pos == len)
		outc(c);
	else
		ed_draw(pos - 1);
}

static void ed_delete_at(unsigned int i)
{
	unsigned int k;

	if (i >= len)
		return;
	for (k = i; k < len; k++)
		line[k] = line[k + 1];
	len--;
	ed_draw(i);
}

static void hist_push(void)
{
	int i;

	if (len == 0)
		return;
	if (hist_n > 0 && strcmp(hist[hist_n - 1], line) == 0)
		return;
	if (hist_n == QB_HIST_N) {
		for (i = 1; i < QB_HIST_N; i++)
			strcpy(hist[i - 1], hist[i]);
		hist_n--;
	}
	strcpy(hist[hist_n++], line);
}

static void hist_move(int dir)
{
	int to = hist_i + dir;

	if (to < 0 || to > hist_n)
		return;
	if (hist_i == hist_n)
		strcpy(stash, line);
	hist_i = to;
	ed_load(hist_i == hist_n ? stash : hist[hist_i]);
}

/* The cursor may rest on column 79 in the "pending wrap" state after a
 * program printed exactly 80 columns; the console reports it as 79.  A
 * non-blank character in that cell means the next write would wrap: start
 * the line on a fresh row. */
static void start_cell(void)
{
	char row[96];
	int r = 0, c = 0, n;

	qd_con_cursor(con, &r, &c);
	if (c >= 79) {
		n = qd_con_snap(con, -1, r, row, (int) sizeof(row));
		if (n >= 80 && row[79] != ' ' && row[79] != 0) {
			outc('\n');
			qd_con_cursor(con, &r, &c);
		}
	}
	sr = r;
	sc = c;
}

/* 1: a line is in line[0..len); 0: end of input; 2: abandoned (Ctrl+C). */
static int edit_line(void)
{
	int k;

	start_cell();
	len = pos = 0;
	line[0] = 0;
	hist_i = hist_n;
	qd_con_show_cursor(con, 1);
	for (;;) {
		k = next_key();
		if (k < 0)
			return 0;
		switch (k) {
		case K_ENTER:
		case K_LF:
			{
				unsigned int off = (unsigned int) sc + len;

				ed_goto(sr + (int) (off / 80), (int) (off % 80));
				outc('\n');
				hist_push();
				return 1;
			}
		case K_CTRL_C:
			{
				unsigned int off = (unsigned int) sc + len;

				ed_goto(sr + (int) (off / 80), (int) (off % 80));
				outc('\n');
				len = pos = 0;
				line[0] = 0;
				return 2;
			}
		case K_CTRL_D:
			if (len == 0)
				return 0;
			if (pos < len)
				ed_delete_at(pos);
			break;
		case K_BS:
		case K_DBS:
			if (pos > 0) {
				pos--;
				ed_delete_at(pos);
			}
			break;
		case QD_KEY_DEL:
			ed_delete_at(pos);
			break;
		case QD_KEY_LEFT:
		case K_CTRL_B:
			if (pos > 0) {
				pos--;
				ed_cursor();
			}
			break;
		case QD_KEY_RIGHT:
		case K_CTRL_F:
			if (pos < len) {
				pos++;
				ed_cursor();
			}
			break;
		case QD_KEY_HOME:
		case K_CTRL_A:
			pos = 0;
			ed_cursor();
			break;
		case QD_KEY_END:
		case K_CTRL_E:
			pos = len;
			ed_cursor();
			break;
		case QD_KEY_UP:
		case K_CTRL_P:
			hist_move(-1);
			break;
		case QD_KEY_DOWN:
		case K_CTRL_N:
			hist_move(1);
			break;
		case K_CTRL_U:
		case QD_KEY_ESC:
			pos = 0;
			len = 0;
			line[0] = 0;
			ed_draw(0);
			break;
		case K_CTRL_K:
			len = pos;
			line[len] = 0;
			ed_draw(pos);
			break;
		default:
			if (k >= 0x20 && k < 0x7F)
				ed_insert(k);
			break;
		}
	}
}

/* Cooked fallback: the kernel's own line discipline echoes and edits. */
static int cooked_line(void)
{
	int n = qd_read(con, line, QB_LINE_MAX);

	if (n <= 0)
		return 0;
	len = (unsigned int) n;
	while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
		len--;
	line[len] = 0;
	return 1;
}

static int fill_pending(void)
{
	int r = editor ? edit_line() : cooked_line();

	if (r == 0)
		return 0;
	if (r == 2) {
		pend[0] = '\n';
		pend_len = 1;
	} else {
		memcpy(pend, line, len);
		pend[len] = '\n';
		pend_len = (int) len + 1;
	}
	pend_pos = 0;
	return 1;
}

int qb_getline(char *dst, int cap)
{
	int n;

	if (pend_pos >= pend_len && !fill_pending())
		return 0;
	n = pend_len - pend_pos;
	if (n > cap)
		n = cap;
	memcpy(dst, pend + pend_pos, (unsigned int) n);
	pend_pos += n;
	return n;
}

/* ---- channel adapters ------------------------------------------------ */

static int con_write(void *obj, const char *s, size_t n, size_t *done)
{
	unsigned int k = n > OUT_CHUNK ? OUT_CHUNK : (unsigned int) n;
	int w;

	(void) obj;
	if (editor)
		poll_keys();
	if (break_flag)
		return -1;                    /* "not complete": the ROM's break error */
	w = qd_write(con, s, k);
	if (w <= 0) {
		*done = 0;
		return SB_E_IO;
	}
	*done = (size_t) w;
	return SB_OK;
}

static int diag_write(void *obj, const char *s, size_t n, size_t *done)
{
	unsigned int k = n > OUT_CHUNK ? OUT_CHUNK : (unsigned int) n;
	int w, old;

	(void) obj;
	old = qd_con_attr(con, QD_ATTR(12, 0));
	w = qd_write(con, s, k);
	qd_con_attr(con, old);
	if (w <= 0) {
		*done = 0;
		return SB_E_IO;
	}
	*done = (size_t) w;
	return SB_OK;
}

static int con_read(void *obj, char *buf, size_t n, size_t *got)
{
	(void) obj;
	*got = 0;
	if (n == 0)
		return SB_OK;
	if (pend_pos >= pend_len && !fill_pending())
		return SB_OK;                 /* end of input */
	if (n > (size_t) (pend_len - pend_pos))
		n = (size_t) (pend_len - pend_pos);
	memcpy(buf, pend + pend_pos, n);
	pend_pos += (int) n;
	*got = n;
	return SB_OK;
}

static const SbChanOps rw_ops = { con_write, con_read, NULL, NULL };
static const SbChanOps wo_ops = { con_write, NULL, NULL, NULL };
static const SbChanOps diag_ops = { diag_write, NULL, NULL, NULL };

int qb_attach(SbState *st)
{
	SbChan *c[SB_NCHAN + 1];
	int i;

	for (i = 0; i < SB_NCHAN; i++)
		c[i] = sb_chan_new(&st->mem, i < 3 ? &rw_ops : &wo_ops, NULL,
		                   i < 3 ? (SB_CH_READ | SB_CH_WRITE) : SB_CH_WRITE);
	c[SB_NCHAN] = sb_chan_new(&st->mem, &diag_ops, NULL, SB_CH_WRITE);
	for (i = 0; i <= SB_NCHAN; i++) {
		if (c[i] == NULL) {
			for (i = 0; i <= SB_NCHAN; i++)
				sb_chan_free(c[i]);
			return SB_E_OUT_OF_MEMORY;
		}
	}
	for (i = 0; i < SB_NCHAN; i++)
		sb_set_channel(st, i, c[i]);
	sb_set_diag(st, c[SB_NCHAN]);
	return SB_OK;
}

void qb_detach(SbState *st)
{
	int i;

	for (i = 0; i < SB_NCHAN; i++) {
		sb_chan_free(st->chan[i]);
		sb_set_channel(st, i, NULL);
	}
	sb_chan_free(st->diag);
	sb_set_diag(st, NULL);
}

/* ---- setup ----------------------------------------------------------- */

int qb_io_editor(void)
{
	return editor;
}

int qb_io_init(void)
{
	int caps;

	con = qd_con_channel();
	if (con < 0)
		return -1;
	caps = qd_con_caps(con);
	editor = (caps >= 0 && (caps & QD_CONCAP_KEYS) != 0 && qd_con_version(con) >= 2);
	if (editor) {
		old_mode = qd_con_mode(con, 0u);           /* raw, no echo */
	} else {
		old_mode = qd_con_mode(con, QD_CON_ECHO | QD_CON_COOKED);
	}
	mode_saved = old_mode >= 0;
	tah_n = tah_r = 0;
	break_flag = 0;
	hist_n = 0;
	pend_len = pend_pos = 0;
	return 0;
}

void qb_io_done(void)
{
	if (mode_saved)
		qd_con_mode(con, (unsigned int) old_mode);
	mode_saved = 0;
}
