/* CONAPI: R12 guest self-check of console API version 2 on the app's own console (C89).  Reads back what the kernel stored with qd_con_snap
 * (no screen needed).  Results are recorded and printed at the end so the CHECK lines do not scroll the console under test.
 * Banner "QDOS/386 R12 CONAPI PASS".  Input checks only look at the queue (nothing is typed), so the program runs unattended. */
#include <stdio.h>
#include <string.h>
#include "qdos386.h"

#define MAXCHK 160
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
	int ch = qd_con_channel(), rows = 0, cols = 0, r = 0, c = 0, old, i, ver, caps;
	char line[8];

	CHECK("console channel open", ch > 0);
	ver = qd_con_version(ch);
	CHECK("API version is 2", ver == QD_CON_API_VERSION);
	caps = qd_con_caps(ch);
	CHECK("caps readable, QUEUED set", caps >= 0 && (caps & QD_CONCAP_QUEUED) != 0);
	CHECK("size 25 x 80", qd_con_size(ch, &rows, &cols) == 0 && rows == 25 && cols == 80);
	CHECK("no key waiting (queue empty)", qd_con_queued(ch) == 0);

	/* origin and absolute positioning: rows and columns are 0-based, row 0 the top line, column 0 the left */
	qd_con_clear(ch, QD_CLR_ALL);
	CHECK("putat writes at (5,10)", qd_con_putat(ch, 5, 10, "HELLO", 5) == 5);
	CHECK("row 5 holds HELLO from column 10", snap_is(ch, 5, "          HELLO"));
	qd_con_cursor(ch, &r, &c);
	CHECK("cursor after putat is (5,15)", r == 5 && c == 15);
	CHECK("putat at the last cell (24,79)", qd_con_putat(ch, 24, 79, "Z", 1) == 1);
	CHECK("last cell holds Z", qd_con_snap(ch, -1, 24, row, sizeof row) == 80 && row[79] == 'Z');
	CHECK("putat row 25 is out of range", qd_con_putat(ch, 25, 0, "x", 1) == QD_ERR_RANGE);
	CHECK("putat col 80 is out of range", qd_con_putat(ch, 0, 80, "x", 1) == QD_ERR_RANGE);
	CHECK("putat row -1 is out of range", qd_con_putat(ch, -1, 0, "x", 1) == QD_ERR_RANGE);
	CHECK("putat NULL text is out of range", qd_con_putat(ch, 0, 0, 0, 1) == QD_ERR_RANGE);
	qd_con_cursor(ch, &r, &c);
	CHECK("a refused putat leaves the cursor alone", r == 24 && c == 79);

	/* clear: EOL, line, EOS, all */
	qd_con_putat(ch, 6, 0, "0123456789", 10);
	qd_con_goto(ch, 6, 4);
	CHECK("clear EOL", qd_con_clear(ch, QD_CLR_EOL) == 0);
	CHECK("row 6 cut after column 3", snap_is(ch, 6, "0123"));
	qd_con_cursor(ch, &r, &c);
	CHECK("clear EOL keeps the cursor", r == 6 && c == 4);
	qd_con_putat(ch, 7, 0, "second line", 11);
	qd_con_goto(ch, 7, 3);
	CHECK("clear LINE", qd_con_clear(ch, QD_CLR_LINE) == 0);
	CHECK("row 7 empty", snap_is(ch, 7, ""));
	qd_con_putat(ch, 8, 0, "eight", 5);
	qd_con_putat(ch, 9, 0, "nine", 4);
	qd_con_goto(ch, 8, 2);
	CHECK("clear EOS", qd_con_clear(ch, QD_CLR_EOS) == 0);
	CHECK("row 8 cut, row 9 empty", snap_is(ch, 8, "ei") && snap_is(ch, 9, ""));
	CHECK("rows above survive", snap_is(ch, 6, "0123") && snap_is(ch, 5, "          HELLO"));
	CHECK("clear ALL", qd_con_clear(ch, QD_CLR_ALL) == 0);
	CHECK("all rows empty after clear", snap_is(ch, 5, "") && snap_is(ch, 6, "") && snap_is(ch, 24, ""));
	qd_con_cursor(ch, &r, &c);
	CHECK("clear ALL homes the cursor", r == 0 && c == 0);
	CHECK("clear with a bad selector", qd_con_clear(ch, 9) == QD_ERR_RANGE);

	/* control characters in the output stream */
	qd_con_clear(ch, QD_CLR_ALL);
	qd_puts("AB\rC");
	CHECK("CR returns to column 0 (CB)", snap_is(ch, 0, "CB"));
	qd_con_clear(ch, QD_CLR_ALL);
	qd_puts("x\ty");
	CHECK("TAB goes to the next multiple of 8", snap_is(ch, 0, "x       y"));
	qd_con_clear(ch, QD_CLR_ALL);
	qd_puts("one\ntwo\n");
	qd_con_cursor(ch, &r, &c);
	CHECK("LF is a new line at column 0", snap_is(ch, 0, "one") && snap_is(ch, 1, "two") && r == 2 && c == 0);

	/* scrolling: 30 lines on a 25-line console */
	qd_con_clear(ch, QD_CLR_ALL);
	for (i = 1; i <= 30; i++) {
		sprintf(line, "L%02d\n", i);
		qd_puts(line);
	}
	CHECK("scrolled: row 0 is L07", snap_is(ch, 0, "L07"));
	CHECK("scrolled: row 23 is L30", snap_is(ch, 23, "L30"));
	CHECK("scrolled: cursor on the last row", qd_con_cursor(ch, &r, &c) >= 0 && r == 24 && c == 0);

	/* attributes and cursor visibility */
	old = qd_con_attr(ch, QD_ATTR(14, 1));
	CHECK("attribute returns the old one", old == 0x07);
	CHECK("new attribute is 0x1E", qd_con_attr(ch, 0x07) == 0x1E);
	CHECK("attribute out of range", qd_con_attr(ch, 256) == QD_ERR_RANGE);
	CHECK("hide cursor", qd_con_show_cursor(ch, 0) == 0 && qd_con_cursor(ch, &r, &c) == 0);
	CHECK("show cursor", qd_con_show_cursor(ch, 1) == 0 && qd_con_cursor(ch, &r, &c) == 1);

	/* modes */
	CHECK("raw mode is the default", qd_con_mode(ch, QD_CON_ECHO) == 0);
	CHECK("echo mode readable", qd_con_mode(ch, QD_CON_ECHO | QD_CON_COOKED) == QD_CON_ECHO);
	CHECK("cooked+echo set", qd_con_mode(ch, 0) == (QD_CON_ECHO | QD_CON_COOKED));
	CHECK("back to raw", qd_con_mode(ch, 0) == 0);
	CHECK("bad mode flag", qd_con_mode(ch, 0x10) == QD_ERR_RANGE);

	/* key constants (documented values) */
	CHECK("key code values", QD_KEY_UP == 0x80 && QD_KEY_DEL == 0x88 && QD_KEY_F1 == 0x90 && QD_KEY_CTRL('S') == 0x13);

	{
		int bad = 0;
		qd_con_clear(ch, QD_CLR_ALL);
		for (i = 0; i < nchk && i < MAXCHK; i++) {
			printf("CHECK %s: %s\n", names[i], oks[i] ? "ok" : "FAIL");
			if (!oks[i])
				bad++;
		}
		printf("CONAPI: %d checks, %d failed (caps 0x%X)\n", nchk, bad, caps);
		if (bad == 0 && nchk <= MAXCHK) {
			printf("QDOS/386 R12 CONAPI PASS\n");
			return 0;
		}
		printf("QDOS/386 R12 CONAPI FAIL\n");
		return 1;
	}
}
