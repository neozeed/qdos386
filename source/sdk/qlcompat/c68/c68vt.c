/*
 * QDOS/386 R14A - C68 / LibVT console emulation for QL C programs: ANSI_conwrite, __ANSICONF__, getch, and the
 * small C68 library calls Hack 1.03's QL port needs (docs/R14A-LIBVT-ANSI.md, docs/R14A-QL-C-ABI.md).
 *
 * Architecture (option A of the R14A handoff): userspace, per process.  The C68 libc variant (libc68.a: sdk/libc/
 * stdio.c built with -DQDOS_C68_CONWRITE) offers console text for stdout/stderr to C68's write vector _conwrite when a
 * program sets it - Hack does `_conwrite = ANSI_conwrite` - with C68 libc 4.24.5's interface and return protocol
 * (sdk/qlcompat/include/ansicondef.h).  ANSI_conwrite interprets a VT100 subset, consumes everything (returns 0) and
 * draws on the process's own console session with the R11M/R12 console calls (qd_con_goto / attr, qd_write).
 * The parser state is per process and survives split writes; the screen itself is kept by the kernel's session, so
 * Task Manager switching keeps it.  No escape sequence reaches the kernel console.
 *
 * Interpreted (every sequence Hack's termqd.c emits, plus the common neighbours):
 *   ESC [ r ; c H / f   cursor position (1-based, default 1; clamped)     ESC [ n A/B/C/D  relative moves (default 1)
 *   ESC [ J (0/1/2)     erase below / above / all (2 also homes, as Hack's CL capability requires)
 *   ESC [ K (0/1/2)     erase right / left / line                         ESC [ m          SGR 0 1 4 5 7 22 24 25 27 30-37 39 40-47 49
 *   ESC [ s / u, ESC 7 / 8   save / restore cursor                        ESC [ = 7 h/l, ESC [ ? 7 h/l   line wrap on/off
 *   ESC D / M / E / c   index / reverse index / next line / reset
 *   CR, LF (column 0 + next line, as the QDOS console), BS, TAB (8), BEL (ignored: no sound); other C0 codes ignored.
 * Anything else: the sequence is consumed and ignored.  At most 8 parameters, each capped at 9999; CAN and SUB abort
 * a sequence; an ESC inside a sequence starts a new one.  A lone ESC consumes the next character as its final byte (a
 * VT100 does the same) - Hack's own termcap sets CD (clear to end of screen) to a bare ESC.
 * Colours: SGR 0 restores the measured QL look (green on black); 30-37 / 40-47 map ANSI to CGA; bold = bright,
 * underline = bright (VGA colour text has no underline), reverse swaps.  Characters are not translated: the QL
 * font's 0x60 (pound sign) shows as code page 437's '`'; 0x7F (QL copyright sign) is ignored by the QDOS/386 console
 * like every control byte.  Hack emits neither 0x7F nor bytes above 0x7E (docs/R14A-LIBVT-ANSI.md section 4).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <qdos386.h>
#include "ansicondef.h"

ANSICONDEF __ANSICONF__ = { VT100, 0 };        /* C68 default (Hack sets the same) */

int qd_con_channel(void);                     /* libc: the process's CON channel */

#define ROWS 25
#define COLS 80
#define NPAR 8
#define FG_DEFAULT 2                          /* green: measured on the QL build (QLBox) */
#define BG_DEFAULT 0

static int vt_inited;
static int con;
static int row, col, wrap_on = 1, pend_wrap;
static int fg = FG_DEFAULT, bg = BG_DEFAULT, bold, ul, blink, rev;
static int s_row, s_col;                      /* saved cursor */
static int st;                                /* 0 text, 1 after ESC, 2 in CSI */
static int par[NPAR], npar, priv;
static int cur_attr = -1, nat_r = -1, nat_c = -1;
static char run[COLS];
static int run_n, run_r, run_c;

static const unsigned char ansi2cga[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

static int attr(void)
{
	int f = ansi2cga[fg & 7] | ((bold || ul) ? 8 : 0), b = ansi2cga[bg & 7];
	if (rev) {
		int t = f & 7;
		f = b | (f & 8);
		b = t;
	}
	return (f & 15) | ((b & 7) << 4) | (blink ? 0x80 : 0);
}

static void flush_run(void)
{
	if (run_n) {
		if (nat_r != run_r || nat_c != run_c)
			qd_con_goto(con, run_r, run_c);
		qd_write(con, run, (unsigned int) run_n);
		nat_r = run_r;
		nat_c = run_c + run_n;
		if (nat_c >= COLS)
			nat_r = -1;                       /* the console's own wrap position: re-position next time */
		run_n = 0;
	}
}

static void set_attr(int a)
{
	if (a != cur_attr) {
		flush_run();
		qd_con_attr(con, a);
		cur_attr = a;
	}
}

static void vt_init(struct UFB *u)
{
	if (vt_inited)
		return;
	vt_inited = 1;
	con = (u != 0 && u->ufbfh > 0) ? (int) u->ufbfh : qd_con_channel();
	qd_con_mode(con, 0);
	qd_con_region(con, 0, ROWS - 1);
	set_attr(attr());
}

static void newline(void)
{
	flush_run();
	col = 0;
	if (row < ROWS - 1)
		row++;
	else {                                    /* scroll the whole screen with the console's own newline */
		set_attr(((ansi2cga[bg & 7]) << 4) | ansi2cga[fg & 7]);
		qd_con_goto(con, ROWS - 1, 0);
		qd_write(con, "\n", 1);
		nat_r = -1;
		cur_attr = -1;
	}
}

static void put_char(int c)
{
	int a = attr();
	if (pend_wrap) {
		pend_wrap = 0;
		newline();
	}
	set_attr(a);
	if (run_n && (row != run_r || col != run_c + run_n))
		flush_run();
	if (!run_n) {
		run_r = row;
		run_c = col;
	}
	run[run_n++] = (char) c;
	if (col < COLS - 1)
		col++;
	else if (wrap_on)
		pend_wrap = 1;                        /* the next character goes to the next line */
	else
		flush_run();                          /* wrap off: the last column is overwritten */
}

static void erase(int r0, int c0, int r1, int c1)  /* inclusive, row-major range */
{
	char blanks[COLS];
	int r;
	flush_run();
	memset(blanks, ' ', sizeof blanks);
	set_attr(attr());
	for (r = r0; r <= r1; r++) {
		int a = r == r0 ? c0 : 0, b = r == r1 ? c1 : COLS - 1;
		if (b < a)
			continue;
		qd_con_goto(con, r, a);
		qd_write(con, blanks, (unsigned int) (b - a + 1));
	}
	nat_r = -1;
}

static void clampcur(void)
{
	if (row < 0) row = 0;
	if (row > ROWS - 1) row = ROWS - 1;
	if (col < 0) col = 0;
	if (col > COLS - 1) col = COLS - 1;
	pend_wrap = 0;
}

static int p(int i, int def)
{
	return (i < npar && par[i] > 0) ? par[i] : def;
}

static void sgr(void)
{
	int i, v;
	if (npar == 0)
		npar = 1, par[0] = 0;
	for (i = 0; i < npar; i++) {
		v = par[i];
		if (v == 0) { fg = FG_DEFAULT; bg = BG_DEFAULT; bold = ul = blink = rev = 0; }
		else if (v == 1) bold = 1;
		else if (v == 4) ul = 1;
		else if (v == 5) blink = 1;
		else if (v == 7) rev = 1;
		else if (v == 22) bold = 0;
		else if (v == 24) ul = 0;
		else if (v == 25) blink = 0;
		else if (v == 27) rev = 0;
		else if (v >= 30 && v <= 37) fg = v - 30;
		else if (v == 39) fg = FG_DEFAULT;
		else if (v >= 40 && v <= 47) bg = v - 40;
		else if (v == 49) bg = BG_DEFAULT;
	}
}

static void reset(void)
{
	fg = FG_DEFAULT; bg = BG_DEFAULT; bold = ul = blink = rev = 0;
	wrap_on = 1;
	row = col = 0;
	pend_wrap = 0;
	erase(0, 0, ROWS - 1, COLS - 1);
}

static void csi(int fin)
{
	int n = p(0, 1);
	flush_run();
	switch (fin) {
	case 'H': case 'f': row = p(0, 1) - 1; col = p(1, 1) - 1; clampcur(); break;
	case 'A': row -= n; clampcur(); break;
	case 'B': row += n; clampcur(); break;
	case 'C': col += n; clampcur(); break;
	case 'D': col -= n; clampcur(); break;
	case 'J':
		n = npar ? par[0] : 0;
		if (n == 0) erase(row, col, ROWS - 1, COLS - 1);
		else if (n == 1) erase(0, 0, row, col);
		else if (n == 2) { erase(0, 0, ROWS - 1, COLS - 1); row = col = 0; pend_wrap = 0; }
		break;
	case 'K':
		n = npar ? par[0] : 0;
		if (n == 0) erase(row, col, row, COLS - 1);
		else if (n == 1) erase(row, 0, row, col);
		else if (n == 2) erase(row, 0, row, COLS - 1);
		break;
	case 'm': sgr(); break;
	case 's': s_row = row; s_col = col; break;
	case 'u': row = s_row; col = s_col; clampcur(); break;
	case 'h': case 'l':
		if ((priv == '=' || priv == '?') && npar >= 1 && par[0] == 7) {
			wrap_on = fin == 'h';
			if (!wrap_on)
				pend_wrap = 0;
		}
		break;
	}
}

static void control(int c)
{
	switch (c) {
	case '\r': flush_run(); col = 0; pend_wrap = 0; break;
	case '\n': pend_wrap = 0; newline(); break;
	case '\b': flush_run(); if (col > 0) col--; pend_wrap = 0; break;
	case '\t': flush_run(); col = (col / 8 + 1) * 8; if (col > COLS - 1) col = COLS - 1; break;
	default: break;                          /* BEL and the rest: nothing (no sound) */
	}
}

/* C68 _conwrite protocol: returns 0 (all consumed and shown) or, when no VT100/ANSI emulation is selected, n (the
 * caller writes the bytes unchanged).  VT52 alone is not emulated here. */
int ANSI_conwrite(struct UFB *u, void *buf, unsigned int n)
{
	const unsigned char *s = (const unsigned char *) buf;
	unsigned int i;
	int c;

	vt_init(u);
	if ((__ANSICONF__.emulation & (VT100 | ANSI1 | ANSI2 | ANSI3 | ANSI4)) == 0) {
		flush_run();
		nat_r = -1;                                /* the console moves on its own: re-position next time */
		return (int) n;
	}
	for (i = 0; i < n; i++) {
		c = s[i];
		if (c == 24 || c == 26) {               /* CAN, SUB: abort a sequence */
			st = 0;
			continue;
		}
		if (c == 27) {
			st = 1;
			continue;
		}
		if (st == 1) {
			st = 0;
			if (c == '[') {
				st = 2; npar = 0; priv = 0; par[0] = 0;
			} else if (c == '7') { s_row = row; s_col = col; }
			else if (c == '8') { flush_run(); row = s_row; col = s_col; clampcur(); }
			else if (c == 'D') { pend_wrap = 0; if (row == ROWS - 1) { int cc = col; newline(); col = cc; } else row++; flush_run(); }
			else if (c == 'M') { flush_run(); if (row > 0) row--; pend_wrap = 0; }
			else if (c == 'E') { pend_wrap = 0; newline(); }
			else if (c == 'c') { flush_run(); reset(); }
			/* any other character after ESC is consumed */
			continue;
		}
		if (st == 2) {
			if (c >= '0' && c <= '9') {
				if (npar == 0) npar = 1;
				if (par[npar - 1] < 9999)
					par[npar - 1] = par[npar - 1] * 10 + (c - '0');
				if (par[npar - 1] > 9999) par[npar - 1] = 9999;
			} else if (c == ';') {
				if (npar == 0) npar = 1;
				if (npar < NPAR) par[npar++] = 0;
			} else if ((c == '=' || c == '?' || c == '>') && npar == 0 && !priv) {
				priv = c;
			} else if (c >= 0x40 && c <= 0x7E) {
				st = 0;
				csi(c);
			} else if (c < 0x20) {
				control(c);                     /* a control inside a sequence acts and the sequence goes on */
			} else if (c >= 0x80) {
				st = 0;                         /* malformed: drop the sequence */
			}
			continue;
		}
		if (c < 0x20)
			control(c);
		else
			put_char(c);
	}
	flush_run();
	/* leave the console's cursor where the program's is */
	if (nat_r != row || nat_c != col) {
		qd_con_goto(con, row, col);
		nat_r = row;
		nat_c = col;
	}
	return 0;
}

/* ---- C68 library calls Hack uses that the QDOS/386 libc does not have ---- */

/* C68 (Lattice) getch: one key from the console, no echo, waits (in the kernel: no polling); like C68 it does not
 * flush stdout (Hack's readchar does).  The PC ENTER code 13 is returned as the QL ENTER code 10. */
int getch(void)
{
	int k = qd_con_getkey(qd_con_channel());
	if (k < 0)
		return EOF;
	return k == '\r' ? '\n' : k;
}

/* C68's getlogin: the QL build greets an unnamed player as "root" without asking (measured under QLBox, the
 * QDOS/386 R14A oracle: docs/R14A-HACK-INTAKE.md).  A USER environment variable is used first by Hack itself. */
char *getlogin(void)
{
	return "root";
}

/* kill: no signals between QDOS/386 processes (the QL build's x68k.c stub did nothing either) */
int kill(int pid, int sig)
{
	(void) pid;
	(void) sig;
	return -1;
}
