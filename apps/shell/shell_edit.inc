/*
 * shell_edit.inc - R12: command-line editor and history (included by shell.c with -DR11M_SHELL, after the directory commands).
 *
 * Keys (the PC keyboard delivers them as single bytes, see docs/R12-CONSOLE-EDITOR-ABI.md): Left Right Home End, Delete, Backspace,
 * Up/Down (history), Enter; Ctrl+A / Ctrl+E (Home / End), Ctrl+B / Ctrl+F (left / right), Ctrl+P / Ctrl+N (history), Ctrl+U (delete the
 * line), Ctrl+K (delete to the end), Esc (delete the line), Ctrl+C (abandon the line).  Other keys are ignored.
 *
 * The editor draws the line from its START cell (the cursor position after the prompt, queried once per line) with absolute cursor
 * moves, so the line may wrap over several rows.  When the end of the line would pass the last row the screen is scrolled up first
 * (the start row moves with it).  A character that does not fit in the SH_LINE_MAX - 1 limit is rejected.
 * History: HIST_N lines, volatile, per shell; a line is stored as typed (quotes kept) before it is executed; an empty line or a
 * repeat of the newest entry is not stored.  The HISTORY command lists it.
 *
 * A kernel without the R12 console API (CHAN_CON_CAPS) gets the plain read_line() of the R8 shell.
 */
#define HIST_N      16
#define K_UP        0x80
#define K_DOWN      0x81
#define K_LEFT      0x82
#define K_RIGHT     0x83
#define K_HOME      0x84
#define K_END       0x85
#define K_DEL       0x88

static char hist[HIST_N][SH_LINE_MAX];
static int hist_n;                           /* entries stored */
static int hist_i;                           /* browsing position: hist_n = the line being typed */
static char stash[SH_LINE_MAX];              /* the line being typed while a history entry is shown */
static int ed_mode = -1;                     /* -1 not probed, 0 plain input, 1 editor */
static unsigned int ed_len, ed_pos;
static int ed_sr, ed_sc;                     /* the cell where line[0] is drawn */

static int ed_ctl(unsigned int op, unsigned int arg)
{
	return (int) qtrap(APP_QT_CH_CTL, (unsigned int) con_ch, op, arg);
}

static void ed_goto(int r, int c)
{
	ed_ctl(0x10u, (unsigned int) r | ((unsigned int) c << 8));
}

static void ed_probe(void)
{
	int v = ed_ctl(0x1Bu, 0);                /* CHAN_CON_CAPS: (version << 16) | features */
	ed_mode = (v >= 0 && (v >> 16) >= 2) ? 1 : 0;
}

/* Make the cell after the last character of a line of n characters fit on the screen: scroll up as needed.  Returns 1 if it scrolled
 * (the whole line must be redrawn). */
static int ed_fit(unsigned int n)
{
	int last = ed_sr + (int) ((unsigned int) ed_sc + n) / 80;
	int d = last - 24, k;

	if (d <= 0)
		return 0;
	if (d > ed_sr)
		d = ed_sr;
	ed_goto(24, 0);
	for (k = 0; k < d; k++)
		outc('\n');
	ed_sr -= d;
	return 1;
}

static void ed_cursor(void)
{
	unsigned int off = (unsigned int) ed_sc + ed_pos;
	ed_goto(ed_sr + (int) (off / 80), (int) (off % 80));
}

/* Redraw line[from..], erase what followed, and put the cursor at ed_pos. */
static void ed_draw(unsigned int from)
{
	unsigned int off;

	if (ed_fit(ed_len))
		from = 0;
	off = (unsigned int) ed_sc + from;
	ed_goto(ed_sr + (int) (off / 80), (int) (off % 80));
	if (from < ed_len)
		out(line + from, ed_len - from);
	/* Clear from the cell AFTER the last character.  The console cursor may be in the pending-wrap state (column 80, reported
	 * as 79), where a clear from "the cursor" would erase the character just drawn in the last column. */
	off = (unsigned int) ed_sc + ed_len;
	ed_goto(ed_sr + (int) (off / 80), (int) (off % 80));
	ed_ctl(0x11u, 3u);                       /* clear from there to the end of the screen */
	ed_cursor();
}

static void ed_load(const char *s)
{
	unsigned int n = 0;
	while (s[n] && n < SH_LINE_MAX - 1) {
		line[n] = s[n];
		n++;
	}
	line[n] = 0;
	ed_len = ed_pos = n;
	ed_draw(0);
}

static void ed_insert(int c)
{
	unsigned int i;
	int scrolled;

	if (ed_len >= SH_LINE_MAX - 1)
		return;                              /* overflow: rejected */
	scrolled = ed_fit(ed_len + 1);
	for (i = ed_len; i > ed_pos; i--)
		line[i] = line[i - 1];
	line[ed_pos] = (char) c;
	ed_len++;
	ed_pos++;
	line[ed_len] = 0;
	if (!scrolled && ed_pos == ed_len)
		outc((char) c);                      /* appending: the console's own cursor is already in place */
	else
		ed_draw(ed_pos - 1);
}

static void ed_delete_at(unsigned int i)
{
	unsigned int k;
	if (i >= ed_len)
		return;
	for (k = i; k < ed_len; k++)
		line[k] = line[k + 1];
	ed_len--;
	ed_draw(i);
}

static void hist_push(void)
{
	int i;
	unsigned int k;

	if (ed_len == 0)
		return;
	if (hist_n > 0 && same(hist[hist_n - 1], line))
		return;
	if (hist_n == HIST_N) {
		for (i = 1; i < HIST_N; i++)
			for (k = 0; k < SH_LINE_MAX; k++)
				hist[i - 1][k] = hist[i][k];
		hist_n--;
	}
	for (k = 0; k <= ed_len; k++)
		hist[hist_n][k] = line[k];
	hist_n++;
}

static void ed_hist_move(int dir)
{
	if (dir < 0) {
		if (hist_i == 0)
			return;
		if (hist_i == hist_n) {
			unsigned int k;
			for (k = 0; k <= ed_len; k++)
				stash[k] = line[k];
		}
		hist_i--;
		ed_load(hist[hist_i]);
	} else {
		if (hist_i >= hist_n)
			return;
		hist_i++;
		ed_load(hist_i == hist_n ? stash : hist[hist_i]);
	}
}

/* Read one edited line into line[].  Returns 0 at end of input. */
static int read_line_ed(void)
{
	int c, v;

	ed_len = ed_pos = 0;
	line[0] = 0;
	stash[0] = 0;
	hist_i = hist_n;
	v = ed_ctl(0x13u, 1u);                   /* VQ_CURSOR: col | row << 8 | visible << 16 */
	ed_sc = v & 0xff;
	ed_sr = (v >> 8) & 0xff;
	for (;;) {
		c = con_getc();
		if (c < 0)
			return 0;
		if (c == '\r' || c == '\n') {
			ed_pos = ed_len;
			ed_cursor();
			outc('\n');
			line[ed_len] = 0;
			hist_push();
			return 1;
		}
		if (c >= 32 && c < 127) {
			ed_insert(c);
		} else if (c == 0x08 || c == 0x7f) {
			if (ed_pos > 0) {
				ed_pos--;
				ed_delete_at(ed_pos);
			}
		} else if (c == K_DEL || c == 0x04) {
			ed_delete_at(ed_pos);
		} else if (c == K_LEFT || c == 0x02) {
			if (ed_pos > 0) {
				ed_pos--;
				ed_cursor();
			}
		} else if (c == K_RIGHT || c == 0x06) {
			if (ed_pos < ed_len) {
				ed_pos++;
				ed_cursor();
			}
		} else if (c == K_HOME || c == 0x01) {
			ed_pos = 0;
			ed_cursor();
		} else if (c == K_END || c == 0x05) {
			ed_pos = ed_len;
			ed_cursor();
		} else if (c == K_UP || c == 0x10) {
			ed_hist_move(-1);
		} else if (c == K_DOWN || c == 0x0e) {
			ed_hist_move(1);
		} else if (c == 0x15 || c == 0x1b) {         /* Ctrl+U, Esc: delete the whole line */
			ed_len = ed_pos = 0;
			line[0] = 0;
			ed_draw(0);
		} else if (c == 0x0b) {                      /* Ctrl+K: delete to the end of the line */
			ed_len = ed_pos;
			line[ed_len] = 0;
			ed_draw(ed_pos);
		} else if (c == 0x03) {                      /* Ctrl+C: abandon */
			ed_pos = ed_len;
			ed_cursor();
			outs("^C\n");
			ed_len = 0;
			line[0] = 0;
			return 1;
		}
	}
}

static void cmd_history(void)
{
	int i;
	for (i = 0; i < hist_n; i++) {
		outnum(i + 1);
		outs("  ");
		outs(hist[i]);
		outc('\n');
	}
	if (hist_n == 0)
		outs("history is empty\n");
}
