/*
 * EDIT.AOU - R12 full-screen text editor for QDOS/386 (strict C89, an ordinary slot-1 / paged application).
 *
 *   EDIT [path]            path: relative to the current directory, or absolute (WIN1:/SRC/HELLO.C or /SRC/HELLO.C); a name with
 *                          blanks goes in "double quotes" at the shell.  No argument: an empty untitled buffer.
 *
 * Everything goes through the logical console API (libqdos qd_con_*: absolute cursor, clear, attribute, raw key codes); no
 * video memory, no kernel hooks.  The text lives in one gap buffer of EDIT_MAX bytes in the program's own image (BSS), so
 * memory is bounded and nothing is allocated from the shared kernel heap.  Files are read completely and CLOSED at once: the
 * editor holds no channel between commands (another job can rename or delete the file while it runs).
 *
 * Keys (raw console mode; see docs/R12-CONSOLE-EDITOR-ABI.md):
 *   arrows, Home/End, PgUp/PgDn      move          F2 / F3        top / end of the file
 *   printable, Tab, Enter            insert        Backspace/Delete   delete (joins lines at the ends)
 *   Ctrl+S  save (asks for a name if untitled)     Ctrl+Q / F10   quit (twice if there are unsaved changes)
 *   Ctrl+K  cut the line into the clipboard         Ctrl+U         paste the clipboard at the cursor
 *   Ctrl+F  find                                    Ctrl+N / F4    find next
 *   Ctrl+G  go to line                              Ctrl+L         redraw        F1  help
 *
 * Policy (stated, not hidden):
 *   - Files over EDIT_MAX bytes (262144) are refused, never truncated.  Files with NUL bytes are refused (binary).  Other bytes are
 *     kept exactly; control and non-ASCII bytes are shown as '.'.
 *   - Line ends: a file whose every line ends in CR LF is edited as LF and saved as CR LF; any other file is kept byte for byte (a
 *     lone CR is just a character).  A missing final newline stays missing.
 *   - Save: the complete text is written to a temporary file "~EDIT.TMP" in the target's directory, flushed and closed; an existing
 *     target is renamed to "~EDIT.BAK", the temporary file is renamed to the target, the backup is removed.  A write error (disk
 *     full, write-protected volume, I/O error) leaves the original untouched and removes the temporary file; a failed rename puts
 *     the original back.  This is NOT atomic across a power cut or a kill between the renames; a leftover ~EDIT.TMP / ~EDIT.BAK
 *     is a complete copy.
 *   - No undo, no auto-indent, no mouse.
 * Exit status: 0 normal; 1 usage/console error; 2 file refused; 3 end of input.
 */
#include <stdio.h>
#include <string.h>
#include "qdos386.h"

int qd_con_channel(void);

#define EDIT_MAX    262144u
#define ROWS        25
#define COLS        80
#define TEXT_ROWS   24
#define PATH_MAX_IN 64
#define CLIP_MAX    4096
#define TABW        8

static unsigned char buf[EDIT_MAX];       /* gap buffer: text before the gap, gap, text after */
static unsigned long gs, ge;              /* gap start (= cursor), gap end; text length = EDIT_MAX - (ge - gs) */
static unsigned long top;                 /* offset of the first character of the top row */
static unsigned long ln, topln;           /* cursor line and top line numbers (1-based) */
static int left;                          /* first visible column (horizontal scroll) */
static int goal;                          /* wanted visual column for Up/Down */
static int modified, crlf, untitled, quit_armed;
static int ch;                            /* the CON channel */
static char path[PATH_MAX_IN + 8];        /* WIN1:... form used for I/O */
static char shown[PATH_MAX_IN + 8];       /* name as typed, for the status line */
static char msg[80];                      /* transient message in the status line */
static int msg_bad;
static char find_text[PATH_MAX_IN];
static char clip[CLIP_MAX];
static unsigned int cliplen;
static int last_was_cut;
static unsigned char rowbuf[COLS + 1];
static unsigned char wbuf[256 + 1];

#define LEN()      (EDIT_MAX - (ge - gs))
#define AT(i)      (((i) < gs) ? buf[(i)] : buf[(i) + (ge - gs)])

static void set_msg(const char *s, int bad)
{
	strncpy(msg, s, sizeof msg - 1);
	msg[sizeof msg - 1] = 0;
	msg_bad = bad;
}

static const char *errtext(int rc)
{
	switch (rc) {
	case -44: return "no such file or directory";
	case -46: case -47: return "invalid name";
	case -43: return "bad cluster chain";
	case -80: return "volume is write-protected";
	case -81: return "disk full";
	case -82: return "directory full";
	case -83: return "already exists";
	case -84: return "file is in use";
	case -85: return "write failed";
	case -87: return "not a directory";
	case -88: return "is a directory";
	case -90: return "path too long or too deep";
	case -91: return "name not allowed";
	case -15: return "not supported by this kernel";
	case -18: return "volume not mounted";
	}
	return "error";
}

static void set_err(const char *what, int rc)
{
	char t[80];

	sprintf(t, "%s: %s (%d)", what, errtext(rc), rc);
	set_msg(t, 1);
}

/* ---- gap buffer ---- */

static void move_gap(unsigned long pos)
{
	unsigned long n;

	if (pos < gs) {
		n = gs - pos;
		memmove(buf + ge - n, buf + pos, n);
		gs -= n;
		ge -= n;
	} else if (pos > gs) {
		n = pos - gs;
		memmove(buf + gs, buf + ge, n);
		gs += n;
		ge += n;
	}
}

static int insert_char(unsigned char c)
{
	if (gs == ge)
		return 0;
	buf[gs++] = c;
	if (c == '\n')
		ln++;
	return 1;
}

static unsigned long line_start(unsigned long pos)
{
	while (pos > 0 && AT(pos - 1) != '\n')
		pos--;
	return pos;
}

static unsigned long line_end(unsigned long pos)       /* offset of the '\n' ending the line, or the length */
{
	unsigned long n = LEN();

	while (pos < n && AT(pos) != '\n')
		pos++;
	return pos;
}

/* Visual column reached after the characters of the line from ls up to pos (tabs expand). */
static int vcol_of(unsigned long ls, unsigned long pos)
{
	int v = 0;

	while (ls < pos) {
		if (AT(ls) == '\t')
			v = (v / TABW + 1) * TABW;
		else
			v++;
		ls++;
	}
	return v;
}

/* The offset in the line [ls, le] whose visual column is the largest one not past want. */
static unsigned long pos_for_vcol(unsigned long ls, unsigned long le, int want)
{
	int v = 0, nv;

	while (ls < le) {
		nv = (AT(ls) == '\t') ? (v / TABW + 1) * TABW : v + 1;
		if (nv > want && v >= want)
			break;
		if (nv > want)
			break;
		v = nv;
		ls++;
	}
	return ls;
}

/* ---- drawing ---- */

static void put_row(int r, const unsigned char *s)
{
	qd_con_putat(ch, r, 0, (const char *) s, COLS);
}

static void draw_line_at(int r, unsigned long pos)      /* pos: start of the line shown on row r (or >= length for an empty row) */
{
	unsigned long n = LEN();
	int v = 0, k;
	unsigned char c;

	memset(rowbuf, ' ', COLS);
	rowbuf[COLS] = 0;
	if (pos <= n) {
		while (pos < n && (c = AT(pos)) != '\n') {
			if (c == '\t') {
				k = (v / TABW + 1) * TABW;
				for (; v < k; v++)
					if (v >= left && v - left < COLS)
						rowbuf[v - left] = ' ';
			} else {
				if (v >= left && v - left < COLS)
					rowbuf[v - left] = (c >= 32 && c < 127) ? c : '.';
				v++;
			}
			if (v - left >= COLS)
				break;
			pos++;
		}
	}
	put_row(r, rowbuf);
}

static void draw_status(void)
{
	char t[100];
	int k, l, w;
	unsigned long ls = line_start(gs);

	memset(rowbuf, ' ', COLS);
	rowbuf[COLS] = 0;
	if (msg[0]) {
		l = (int) strlen(msg);
		if (l > COLS)
			l = COLS;
		memcpy(rowbuf, msg, (size_t) l);
	} else {
		w = (int) strlen(shown);
		if (w > 34) {
			sprintf(t, "..%s", shown + w - 32);
		} else {
			strcpy(t, untitled ? "(untitled)" : shown);
		}
		sprintf((char *) rowbuf, "%s%s  Ln %lu Col %d  %s  F1 help", t, modified ? " [+]" : "", ln, vcol_of(ls, gs) + 1, crlf ? "CRLF" : "LF");
		l = (int) strlen((char *) rowbuf);
		for (k = l; k < COLS; k++)
			rowbuf[k] = ' ';
		rowbuf[COLS] = 0;
	}
	qd_con_attr(ch, msg_bad && msg[0] ? QD_ATTR(15, 4) : QD_ATTR(0, 7));
	put_row(ROWS - 1, rowbuf);
	qd_con_attr(ch, 0x07);
}

static void scroll_to_cursor(void)
{
	unsigned long ls = line_start(gs), p;
	int vc = vcol_of(ls, gs);

	if (vc < left)
		left = vc;
	else if (vc >= left + COLS)
		left = vc - COLS + 1;
	if (ln < topln) {
		top = ls;
		topln = ln;
	} else if (ln >= topln + TEXT_ROWS) {
		unsigned long d = ln - (topln + TEXT_ROWS - 1);

		p = top;
		while (d > 0) {
			p = line_end(p);
			if (p < LEN())
				p++;
			topln++;
			d--;
		}
		top = p;
	}
}

static void place_cursor(void)
{
	unsigned long ls = line_start(gs), p = top;
	int r = 0, vc = vcol_of(ls, gs);

	while (p < ls && r < TEXT_ROWS) {
		p = line_end(p);
		p++;
		r++;
	}
	qd_con_goto(ch, r < TEXT_ROWS ? r : TEXT_ROWS - 1, vc - left < 0 ? 0 : (vc - left >= COLS ? COLS - 1 : vc - left));
}

static void redraw(void)
{
	unsigned long p = top, n = LEN();
	int r;

	scroll_to_cursor();
	p = top;
	for (r = 0; r < TEXT_ROWS; r++) {
		if (p > n) {
			draw_line_at(r, n + 1);
		} else {
			draw_line_at(r, p);
			p = line_end(p);
			p = (p < n) ? p + 1 : n + 1;
		}
	}
	draw_status();
	place_cursor();
}

/* Redraw only the cursor's row and the status line (an edit inside one line that did not scroll). */
static void redraw_line(void)
{
	unsigned long ls = line_start(gs), p = top;
	int r = 0, oldleft = left;

	scroll_to_cursor();
	if (left != oldleft) {
		redraw();
		return;
	}
	while (p < ls && r < TEXT_ROWS) {
		p = line_end(p) + 1;
		r++;
	}
	if (r >= TEXT_ROWS || ln < topln) {
		redraw();
		return;
	}
	draw_line_at(r, ls);
	draw_status();
	place_cursor();
}

/* ---- keys and prompts ---- */

static int getkey(void)
{
	return qd_con_getkey(ch);
}

/* A one-line prompt in the status row.  Returns the length (>= 0) or -1 if cancelled (Esc) or input ended. */
static int prompt(const char *label, char *out, int max)
{
	int n = (int) strlen(out), k, l;
	int c;

	for (;;) {
		memset(rowbuf, ' ', COLS);
		rowbuf[COLS] = 0;
		l = sprintf((char *) rowbuf, "%s%s", label, out);
		rowbuf[l] = ' ';
		qd_con_attr(ch, QD_ATTR(0, 7));
		put_row(ROWS - 1, rowbuf);
		qd_con_attr(ch, 0x07);
		qd_con_goto(ch, ROWS - 1, l < COLS ? l : COLS - 1);
		c = getkey();
		if (c < 0)
			return -1;
		if (c == '\r') {
			return n;
		}
		if (c == 0x1b)
			return -1;
		if ((c == 8 || c == 0x7f) && n > 0) {
			out[--n] = 0;
		} else if (c >= 32 && c < 127 && n < max - 1) {
			out[n++] = (char) c;
			out[n] = 0;
		} else if (c == 0x15) {
			n = 0;
			out[0] = 0;
		}
		k = 0;
		(void) k;
	}
}

/* ---- cursor movement ---- */

static void cur_left(void)
{
	if (gs > 0)
		move_gap(gs - 1);
	if (gs < LEN() && AT(gs) == '\n')
		ln--;
}

static void cur_right(void)
{
	if (gs < LEN()) {
		if (AT(gs) == '\n')
			ln++;
		move_gap(gs + 1);
	}
}

static void cur_up(void)
{
	unsigned long ls = line_start(gs), ps;

	if (ls == 0)
		return;
	ps = line_start(ls - 1);
	move_gap(pos_for_vcol(ps, ls - 1, goal));
	ln--;
}

static void cur_down(void)
{
	unsigned long le = line_end(gs), ne;

	if (le >= LEN())
		return;
	ne = line_end(le + 1);
	move_gap(pos_for_vcol(le + 1, ne, goal));
	ln++;
}

static void goto_line_no(unsigned long target)
{
	unsigned long p = 0, n = LEN(), l = 1;

	while (l < target && p < n) {
		p = line_end(p);
		if (p >= n)
			break;
		p++;
		l++;
	}
	move_gap(p);
	ln = l;
}

static void set_goal(void)
{
	goal = vcol_of(line_start(gs), gs);
}

/* ---- editing ---- */

static void do_backspace(void)
{
	if (gs == 0)
		return;
	if (buf[gs - 1] == '\n')
		ln--;
	gs--;
	modified = 1;
}

static void do_delete(void)
{
	if (ge < EDIT_MAX) {
		ge++;
		modified = 1;
	}
}

static void cut_line(void)
{
	unsigned long ls = line_start(gs), le = line_end(gs), n = LEN(), i, take;

	if (!last_was_cut)
		cliplen = 0;
	take = (le < n) ? le + 1 - ls : le - ls;
	if (take == 0)
		return;
	if (cliplen + take > CLIP_MAX) {
		set_msg("clipboard full", 1);
		return;
	}
	for (i = 0; i < take; i++)
		clip[cliplen++] = (char) AT(ls + i);
	move_gap(ls);
	ge += take;
	modified = 1;
}

static void paste(void)
{
	unsigned int i;

	for (i = 0; i < cliplen; i++)
		if (!insert_char((unsigned char) clip[i])) {
			set_msg("buffer full", 1);
			break;
		}
	if (cliplen)
		modified = 1;
}

static int match_at(unsigned long pos, const char *s, size_t m)
{
	size_t i;

	for (i = 0; i < m; i++)
		if (AT(pos + i) != (unsigned char) s[i])
			return 0;
	return 1;
}

static void find_next(void)
{
	unsigned long n = LEN(), p, start = gs + 1;
	size_t m = strlen(find_text);
	int wrapped = 0;

	if (m == 0) {
		set_msg("nothing to find (Ctrl+F)", 1);
		return;
	}
	p = start;
	for (;;) {
		if (p + m > n) {
			if (wrapped)
				break;
			wrapped = 1;
			p = 0;
			continue;
		}
		if (wrapped && p > gs)
			break;
		if (match_at(p, find_text, m)) {
			unsigned long l = 1, q = 0;

			while (q < p) {                 /* the line number of the match */
				if (AT(q) == '\n')
					l++;
				q++;
			}
			move_gap(p);
			ln = l;
			set_msg(wrapped ? "found (wrapped around)" : "found", 0);
			return;
		}
		p++;
	}
	set_msg("not found", 1);
}

/* ---- file I/O ---- */

static int make_path(const char *in, char *out)
{
	size_t l = strlen(in);
	int i;
	char pre[6];

	if (l == 0 || l > PATH_MAX_IN)
		return -90;
	for (i = 0; i < 5; i++) {
		char c = in[i];

		pre[i] = (char) ((c >= 'a' && c <= 'z') ? c - 32 : c);
	}
	pre[5] = 0;
	if (l >= 5 && strncmp(pre, "WIN1", 4) == 0 && (pre[4] == ':' || pre[4] == '_')) {
		strcpy(out, in);
		return 0;
	}
	strcpy(out, "WIN1:");
	strcat(out, in);
	return 0;
}

static int load_file(void)
{
	int fd, rc;
	long size;
	unsigned long got = 0, i, n, w, crs = 0, lfs = 0, tail;
	int rd;

	fd = qd_open_ex(path, QD_O_READ | QD_O_FILE);
	if (fd < 0)
		return fd;
	size = qd_fsize(fd);
	if (size < 0) {
		qd_close(fd);
		return (int) size;
	}
	if ((unsigned long) size > EDIT_MAX) {
		qd_close(fd);
		return 1000;                      /* too large */
	}
	tail = EDIT_MAX - (unsigned long) size;
	while (got < (unsigned long) size) {
		rd = qd_read(fd, buf + tail + got, (unsigned int) (((unsigned long) size - got) > 256u ? 256u : (unsigned long) size - got));
		if (rd < 0) {
			rc = rd;
			qd_close(fd);
			return rc;
		}
		if (rd == 0)
			break;
		got += (unsigned long) rd;
	}
	qd_close(fd);
	n = got;
	for (i = 0; i < n; i++) {
		unsigned char c = buf[tail + i];

		if (c == 0)
			return 1001;                 /* binary */
		if (c == '\n') {
			if (i > 0 && buf[tail + i - 1] == '\r')
				crs++;
			else
				lfs++;
		}
	}
	crlf = (crs > 0 && lfs == 0);
	if (crlf) {                           /* edit as LF: drop the CR of every CR LF, in place */
		w = tail;
		for (i = 0; i < n; i++) {
			unsigned char c = buf[tail + i];

			if (c == '\r' && i + 1 < n && buf[tail + i + 1] == '\n')
				continue;
			buf[w++] = c;
		}
		n = w - tail;
		/* the text now occupies [tail, tail + n); move it to the end of the buffer so the gap is at the front */
		memmove(buf + EDIT_MAX - n, buf + tail, n);
		tail = EDIT_MAX - n;
	}
	gs = 0;
	ge = tail;
	return 0;
}

static unsigned long written_bytes;                /* bytes of the last successful save */

static int write_all(int fd, unsigned long n)    /* the text, CR LF expanded when crlf; 0 or a negative error */
{
	unsigned long i = 0;
	int k, rc;

	written_bytes = 0;
	while (i < n) {
		k = 0;
		while (i < n && k < 250) {
			unsigned char c = AT(i);

			if (crlf && c == '\n') {
				wbuf[k++] = '\r';
				written_bytes++;
			}
			written_bytes++;
			wbuf[k++] = c;
			i++;
		}
		rc = qd_write_all(fd, wbuf, (unsigned int) k);
		if (rc < 0)
			return rc;
		if (rc != k)
			return -85;
	}
	return 0;
}

/* tmp / bak names next to the target */
static void sibling(const char *target, const char *name, char *out)
{
	const char *s = strrchr(target, '/');
	size_t l;

	if (s == 0)
		s = strrchr(target, ':');
	if (s == 0) {
		strcpy(out, name);
		return;
	}
	l = (size_t) (s - target) + 1;
	memcpy(out, target, l);
	strcpy(out + l, name);
}

static int exists(const char *p)
{
	struct qd_stat st;

	return qd_stat(p, &st) == 0;
}

static int save_file(void)
{
	char tmp[PATH_MAX_IN + 24], bak[PATH_MAX_IN + 24];
	int fd, rc, had;
	unsigned long n = LEN();

	sibling(path, "~EDIT.TMP", tmp);
	sibling(path, "~EDIT.BAK", bak);
	if (exists(tmp))
		qd_remove(tmp);
	fd = qd_open_ex(tmp, QD_O_WRITE | QD_O_CREATE | QD_O_EXCL | QD_O_FILE);
	if (fd < 0) {
		set_err("cannot create the temporary file; not saved", fd);
		return fd;
	}
	rc = write_all(fd, n);
	if (rc == 0)
		rc = qd_flush(fd);
	if (rc < 0) {
		qd_close(fd);
		qd_remove(tmp);
		set_err("write error; the original is unchanged", rc);
		return rc;
	}
	rc = qd_close(fd);
	if (rc < 0) {
		qd_remove(tmp);
		set_err("close error; the original is unchanged", rc);
		return rc;
	}
	had = exists(path);
	if (had) {
		if (exists(bak))
			qd_remove(bak);
		rc = qd_rename(path, bak);
		if (rc < 0) {
			qd_remove(tmp);
			set_err("cannot replace the file; the original is unchanged", rc);
			return rc;
		}
	}
	rc = qd_rename(tmp, path);
	if (rc < 0) {
		if (had)
			qd_rename(bak, path);
		qd_remove(tmp);
		set_err("cannot rename the new file; the original is restored", rc);
		return rc;
	}
	if (had && qd_remove(bak) < 0) {
		set_msg("saved (could not remove ~EDIT.BAK)", 1);
		modified = 0;
		return 0;
	}
	return 0;
}

static void do_save(void)
{
	char t[80];
	int was_untitled = untitled;
	char oldpath[sizeof path], oldshown[sizeof shown];

	strcpy(oldpath, path);
	strcpy(oldshown, shown);
	if (untitled) {
		char name[PATH_MAX_IN];

		name[0] = 0;
		if (prompt("Save as: ", name, (int) sizeof name) <= 0) {
			set_msg("save cancelled", 1);
			return;
		}
		if (make_path(name, path) != 0) {
			strcpy(path, oldpath);
			set_err("bad name", -90);
			return;
		}
		strcpy(shown, name);
		untitled = 0;
	}
	if (save_file() == 0) {
		modified = 0;
		quit_armed = 0;
		if (!(msg[0] && msg_bad)) {
			sprintf(t, "saved %lu bytes%s", written_bytes, crlf ? " (CRLF)" : "");
			set_msg(t, 0);
		}
	} else {
		if (was_untitled) {                /* the name did not work: stay untitled so Ctrl+S asks again */
			untitled = 1;
			strcpy(path, oldpath);
			strcpy(shown, oldshown);
		}
		if (!msg[0])
			set_msg("save failed", 1);
	}
}

static const char *help_lines[] = {
	"EDIT - QDOS/386 text editor",
	"",
	"  Arrows, Home, End     move              PgUp, PgDn   one screen up / down",
	"  F2 / F3               top / end         Ctrl+G       go to line number",
	"  Enter, Tab            insert            Backspace    delete left (joins)",
	"  Delete                delete right      Ctrl+K       cut line (repeat: more)",
	"  Ctrl+U                paste             Ctrl+F       find; Ctrl+N/F4: next",
	"  Ctrl+S                save              Ctrl+L       redraw the screen",
	"  Ctrl+Q or F10         quit (press twice with unsaved changes)",
	"",
	"  Save writes ~EDIT.TMP next to the file, then swaps it in; a write error",
	"  keeps the original.  Files over 262144 bytes or with NUL bytes are",
	"  refused, not edited.  CRLF files stay CRLF.  No undo.",
	0
};

static void show_help(void)
{
	int r;
	const char *l;

	qd_con_clear(ch, QD_CLR_ALL);
	for (r = 0; (l = help_lines[r]) != 0; r++)
		qd_con_putat(ch, 1 + r, 2, l, (int) strlen(l));
	qd_con_putat(ch, 22, 2, "press any key", 13);
	getkey();
	qd_con_clear(ch, QD_CLR_ALL);
}

static void clear_msg(void)
{
	msg[0] = 0;
	msg_bad = 0;
}

int main(int argc, char **argv)
{
	int c, rc, full, rows = 0, cols = 0;

	ch = qd_con_channel();
	if (ch <= 0 || qd_con_size(ch, &rows, &cols) != 0 || rows != ROWS || cols != COLS) {
		printf("EDIT: needs the R12 console (80x25)\n");
		return 1;
	}
	if (argc > 2) {
		printf("usage: EDIT [path]\n");
		return 1;
	}
	gs = 0;
	ge = EDIT_MAX;
	ln = topln = 1;
	untitled = 1;
	if (argc == 2) {
		rc = make_path(argv[1], path);
		if (rc != 0) {
			printf("EDIT: path too long or empty (limit %d characters)\n", PATH_MAX_IN);
			return 1;
		}
		strncpy(shown, argv[1], sizeof shown - 1);
		untitled = 0;
		rc = load_file();
		if (rc == 1000) {
			printf("EDIT: %s is larger than %lu bytes: not opened\n", argv[1], (unsigned long) EDIT_MAX);
			return 2;
		}
		if (rc == 1001) {
			printf("EDIT: %s contains NUL bytes (binary): not opened\n", argv[1]);
			return 2;
		}
		if (rc == -44) {
			gs = 0;
			ge = EDIT_MAX;
			set_msg("new file", 0);
		} else if (rc < 0) {
			printf("EDIT: cannot open %s: %s (%d)\n", argv[1], errtext(rc), rc);
			return 2;
		}
	}
	qd_con_mode(ch, 0);                   /* raw: key codes, no echo */
	qd_con_clear(ch, QD_CLR_ALL);
	redraw();
	for (;;) {
		c = getkey();
		if (c < 0) {
			qd_con_clear(ch, QD_CLR_ALL);
			printf("EDIT: end of input\n");
			return 3;
		}
		full = 1;
		if (c == QD_KEY_CTRL('Q') || c == QD_KEY_F1 + 9) {
			if (modified && !quit_armed) {
				set_msg("unsaved changes: Ctrl+Q again to quit without saving, any other key to go on", 1);
				quit_armed = 1;
				draw_status();
				place_cursor();
				continue;
			}
			break;
		}
		if (quit_armed) {
			quit_armed = 0;
			clear_msg();
		}
		if (msg[0] && c != QD_KEY_CTRL('S'))
			clear_msg();
		last_was_cut = (c == QD_KEY_CTRL('K'));
		switch (c) {
		case QD_KEY_LEFT:  cur_left(); set_goal(); break;
		case QD_KEY_RIGHT: cur_right(); set_goal(); break;
		case QD_KEY_UP:    cur_up(); break;
		case QD_KEY_DOWN:  cur_down(); break;
		case QD_KEY_HOME: case 0x01:
			move_gap(line_start(gs)); set_goal(); break;
		case QD_KEY_END: case 0x05:
			move_gap(line_end(gs)); set_goal(); break;
		case QD_KEY_PGUP: {
			int k;
			for (k = 0; k < TEXT_ROWS - 1; k++)
				cur_up();
			break;
		}
		case QD_KEY_PGDN: {
			int k;
			for (k = 0; k < TEXT_ROWS - 1; k++)
				cur_down();
			break;
		}
		case QD_KEY_F1 + 1: move_gap(0); ln = 1; set_goal(); break;
		case QD_KEY_F1 + 2: move_gap(LEN()); goto_line_no(0x7fffffffu); set_goal(); break;
		case QD_KEY_F1: show_help(); break;
		case 0x0c: qd_con_clear(ch, QD_CLR_ALL); break;
		case '\r':
			if (!insert_char('\n'))
				set_msg("buffer full (262144 bytes)", 1);
			else
				modified = 1;
			set_goal();
			break;
		case '\t':
			if (!insert_char('\t'))
				set_msg("buffer full (262144 bytes)", 1);
			else {
				modified = 1;
				set_goal();
				full = 0;
			}
			break;
		case 8: case 0x7f:
			if (gs > 0 && buf[gs - 1] != '\n')
				full = 0;
			do_backspace();
			set_goal();
			break;
		case QD_KEY_DEL:
			if (ge < EDIT_MAX && buf[ge] != '\n')
				full = 0;
			do_delete();
			break;
		case 0x13: do_save(); break;
		case 0x0b: cut_line(); set_goal(); break;
		case 0x15: paste(); set_goal(); break;
		case 0x06: {
			char t[PATH_MAX_IN];

			strcpy(t, find_text);
			if (prompt("Find: ", t, (int) sizeof t) > 0) {
				strcpy(find_text, t);
				find_next();
			}
			set_goal();
			break;
		}
		case 0x0e: case QD_KEY_F1 + 3: find_next(); set_goal(); break;
		case 0x07: {
			char t[24];

			t[0] = 0;
			if (prompt("Go to line: ", t, (int) sizeof t) > 0) {
				unsigned long v = 0;
				int i;

				for (i = 0; t[i] >= '0' && t[i] <= '9' && i < 9; i++)
					v = v * 10 + (unsigned long) (t[i] - '0');
				if (v == 0)
					set_msg("not a line number", 1);
				else
					goto_line_no(v);
			}
			set_goal();
			break;
		}
		default:
			if (c >= 32 && c < 127) {
				if (!insert_char((unsigned char) c))
					set_msg("buffer full (262144 bytes)", 1);
				else {
					modified = 1;
					set_goal();
					full = 0;
				}
			}
			break;
		}
		if (full)
			redraw();
		else
			redraw_line();
	}
	qd_con_clear(ch, QD_CLR_ALL);
	qd_con_show_cursor(ch, 1);
	return 0;
}
