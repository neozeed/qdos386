/* CONT1: R11M guest self-check of the virtual console API on the app's own console (C89).
 * Uses qd_con_snap to read back what the kernel stored, so it needs no screen.  Banner "QDOS/386 R11M CONT1 PASS". */
#include <stdio.h>
#include <string.h>
#include "qdos386.h"

/* The checks inspect the console that the program itself prints on, so the results are recorded and printed at the end
 * (printing a CHECK line as it happens would scroll the console under test). */
#define MAXCHK 128
static const char *names[MAXCHK];
static unsigned char oks[MAXCHK];
static int nchk = 0;

static void check(const char *name, int ok)
{
	if (nchk < MAXCHK) {
		names[nchk] = name;
		oks[nchk] = (unsigned char) (ok ? 1 : 0);
	}
	nchk++;
}
#define CHECK(name, cond) check((name), (cond) ? 1 : 0)

int qd_con_channel(void);

static char row[100];

static int snap_is(int ch, int r, const char *want)
{
	int n = qd_con_snap(ch, -1, r, row, sizeof row);
	return n >= 0 && strcmp(row, want) == 0;
}

int main(void)
{
	int ch = qd_con_channel(), rows = 0, cols = 0, r = 0, c = 0, old, i;
	char big[100];

	CHECK("console channel open", ch > 0);
	CHECK("console number 0..3", qd_con_number(ch) >= 0 && qd_con_number(ch) < QD_CON_MAX);
	CHECK("size 25 x 80", qd_con_size(ch, &rows, &cols) == 0 && rows == 25 && cols == 80);

	CHECK("clear all", qd_con_clear(ch, QD_CLR_ALL) == 0);
	qd_con_cursor(ch, &r, &c);
	CHECK("clear homes the cursor", r == 0 && c == 0);
	qd_puts("hello");
	qd_con_cursor(ch, &r, &c);
	CHECK("cursor after text", r == 0 && c == 5);
	CHECK("snap row 0", snap_is(ch, 0, "hello"));

	CHECK("goto 0-based 3,10", qd_con_goto(ch, 3, 10) == 0);
	qd_con_cursor(ch, &r, &c);
	CHECK("cursor is 3,10", r == 3 && c == 10);
	qd_puts("X");
	CHECK("text lands at 3,10", snap_is(ch, 3, "          X"));
	CHECK("gotoxy1 is 1-based", qd_con_gotoxy1(ch, 1, 1) == 0);
	qd_con_cursor(ch, &r, &c);
	CHECK("gotoxy1(1,1) is 0,0", r == 0 && c == 0);
	qd_puts("J");
	CHECK("overwrite at 0,0", snap_is(ch, 0, "Jello"));

	CHECK("goto row 25 refused", qd_con_goto(ch, 25, 0) == QD_ERR_RANGE);
	CHECK("goto col 80 refused", qd_con_goto(ch, 0, 80) == QD_ERR_RANGE);
	CHECK("goto negative refused", qd_con_goto(ch, -1, 0) == QD_ERR_RANGE);
	CHECK("gotoxy1(0,0) refused", qd_con_gotoxy1(ch, 0, 0) == QD_ERR_RANGE);
	CHECK("bad clear refused", qd_con_clear(ch, 9) == QD_ERR_RANGE);
	CHECK("bad attr refused", qd_con_attr(ch, 256) == QD_ERR_RANGE);
	CHECK("bad mode refused", qd_con_mode(ch, 0x80u) == QD_ERR_RANGE);
	CHECK("bad focus refused", qd_con_focus(ch, 4) == QD_ERR_RANGE);
	CHECK("bad region refused", qd_con_region(ch, 5, 2) == QD_ERR_RANGE && qd_con_region(ch, 0, 25) == QD_ERR_RANGE);
	CHECK("snap bad row refused", qd_con_snap(ch, -1, 25, row, sizeof row) == QD_ERR_RANGE);
	CHECK("snap bad console refused", qd_con_snap(ch, 4, 0, row, sizeof row) == QD_ERR_RANGE);
	CHECK("snap null buffer refused", qd_con_snap(ch, -1, 0, 0, 10) == QD_ERR_RANGE);

	qd_con_goto(ch, 0, 3);
	CHECK("clear to end of line", qd_con_clear(ch, QD_CLR_EOL) == 0 && snap_is(ch, 0, "Jel"));
	qd_con_goto(ch, 3, 0);
	CHECK("clear line", qd_con_clear(ch, QD_CLR_LINE) == 0 && snap_is(ch, 3, ""));
	qd_con_goto(ch, 0, 0);
	qd_puts("ABC\nDEF");
	qd_con_goto(ch, 0, 1);
	CHECK("clear to end of screen", qd_con_clear(ch, QD_CLR_EOS) == 0 && snap_is(ch, 0, "A") && snap_is(ch, 1, ""));

	old = qd_con_attr(ch, QD_ATTR(14, 1));
	CHECK("attr returns the previous attribute", old >= 0);
	CHECK("attr set reads back", qd_con_attr(ch, old) == QD_ATTR(14, 1));

	CHECK("hide cursor", qd_con_show_cursor(ch, 0) == 0 && qd_con_cursor(ch, 0, 0) == 0);
	CHECK("show cursor", qd_con_show_cursor(ch, 1) == 0 && qd_con_cursor(ch, 0, 0) == 1);

	qd_con_clear(ch, QD_CLR_ALL);
	CHECK("region 2..4", qd_con_region(ch, 2, 4) == 0);
	qd_con_goto(ch, 0, 0);
	qd_puts("top");
	qd_con_goto(ch, 2, 0);
	qd_puts("r2\nr3\nr4\nr5");
	CHECK("rows outside the region stay", snap_is(ch, 0, "top"));
	CHECK("region scrolled within itself", snap_is(ch, 2, "r3") && snap_is(ch, 3, "r4") && snap_is(ch, 4, "r5"));
	CHECK("region 0..24 restores", qd_con_region(ch, 0, 24) == 0);

	qd_con_clear(ch, QD_CLR_ALL);
	for (i = 0; i < 30; i++) {
		sprintf(big, "line%d\n", i);
		qd_puts(big);
	}
	CHECK("whole console scrolls: top line is line6", snap_is(ch, 0, "line6"));
	CHECK("last text line is line29", snap_is(ch, 23, "line29"));

	qd_con_clear(ch, QD_CLR_ALL);
	qd_puts("a\tb");
	CHECK("tab advances to column 8", snap_is(ch, 0, "a       b"));
	qd_puts("\rZ");
	CHECK("CR returns to column 0", snap_is(ch, 0, "Z       b"));
	qd_puts("\b\bQ");
	qd_con_cursor(ch, &r, &c);
	CHECK("backspace moves left", c == 1 && snap_is(ch, 0, "Q       b"));

	old = qd_con_mode(ch, QD_CON_ECHO);
	CHECK("mode default is raw", old == 0);
	CHECK("mode reads back", qd_con_mode(ch, 0) == QD_CON_ECHO);

	CHECK("diag read", qd_con_diag(ch, -1) >= 0);
	CHECK("diag bad mode refused", qd_con_diag(ch, 3) == QD_ERR_RANGE);

	qd_con_clear(ch, QD_CLR_ALL);
	{
		int bad = 0;

		if (nchk > MAXCHK)
			nchk = MAXCHK;
		for (i = 0; i < nchk; i++) {
			printf("CHECK %s: %s\n", names[i], oks[i] ? "ok" : "FAIL");
			if (!oks[i])
				bad++;
		}
		printf("CONT1: %d checks passed, %d failed\n", nchk - bad, bad);
		if (bad == 0) {
			printf("QDOS/386 R11M CONT1 PASS\n");
			return 0;
		}
		printf("QDOS/386 R11M CONT1 FAIL\n");
		return 1;
	}
}
