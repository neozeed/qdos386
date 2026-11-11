/*
 * shell_r13b.inc - R13B shell commands (-DR13B_SHELL): CLS, DATE, TIME and programs run by name
 * (docs/R13B-EXEC-RESOLUTION-ABI.md, docs/R13B-CLOCK-ABI.md).  The same source is the boot shell (SHELL.AOU, linked at
 * 0x200000) and the dynamic shell (SH.AOU, -DR13B_DYN, linked at the canonical 0x240000 like every program).
 */
#define R13B_QT_WALL   0x23u        /* QT_WALL_TIME: seconds since 1970-01-01 UTC, 0xFFFFFFFF = no clock */
#define R13B_CC_CLEAR  0x11u        /* console control: clear (0 = whole session, cursor home) */

/* CLS: clear this shell's own session (its cells; the display only if it is shown), cursor to row 0, column 0.  The
 * attribute, font, text/rendered mode and every other session are untouched. */
static void cmd_cls(char *arg)
{
	if (*arg) {
		outs("usage: CLS\n");
		return;
	}
	if (capturing)
		return;
	(void) qtrap(APP_QT_CH_CTL, (unsigned int) con_ch, R13B_CC_CLEAR, 0u);
}

/* UTC calendar from a time value (days since 1970-01-01: the civil-from-days algorithm, all values unsigned). */
static void r13b_civil(unsigned long t, unsigned long *y, unsigned int *m, unsigned int *d, unsigned int *wd,
                       unsigned int *hh, unsigned int *mm, unsigned int *ss)
{
	unsigned long days = t / 86400ul, sec = t % 86400ul, z, era, doe, yoe, doy, mp;

	*hh = (unsigned int) (sec / 3600ul);
	*mm = (unsigned int) (sec / 60ul % 60ul);
	*ss = (unsigned int) (sec % 60ul);
	*wd = (unsigned int) ((days + 4ul) % 7ul);          /* 1970-01-01 was a Thursday (4) */
	z = days + 719468ul;                                /* days since 0000-03-01 */
	era = z / 146097ul;
	doe = z - era * 146097ul;
	yoe = (doe - doe / 1460ul + doe / 36524ul - doe / 146096ul) / 365ul;
	doy = doe - (365ul * yoe + yoe / 4ul - yoe / 100ul);
	mp = (5ul * doy + 2ul) / 153ul;
	*d = (unsigned int) (doy - (153ul * mp + 2ul) / 5ul + 1ul);
	*m = (unsigned int) (mp < 10ul ? mp + 3ul : mp - 9ul);
	*y = yoe + era * 400ul + (*m <= 2u ? 1ul : 0ul);
}

static void out2(unsigned int v)
{
	outc((char) ('0' + (int) (v / 10u % 10u)));
	outc((char) ('0' + (int) (v % 10u)));
}

/* DATE / TIME: display only.  Nothing is invented when the kernel has no clock. */
static void cmd_datetime(char *arg, int want_date)
{
	static const char *const wdn[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	unsigned long t, y;
	unsigned int m, d, wd, hh, mm, ss;

	if (*arg) {
		outs(want_date ? "usage: DATE   (display only: the date comes from the machine's clock)\n"
		               : "usage: TIME   (display only: the time comes from the machine's clock)\n");
		return;
	}
	t = (unsigned long) qtrap(R13B_QT_WALL, 0, 0, 0);
	if (t == 0xFFFFFFFFul) {
		outs(want_date ? "DATE: unavailable - this machine has no wall clock configured\n"
		               : "TIME: unavailable - this machine has no wall clock configured\n");
		return;
	}
	r13b_civil(t, &y, &m, &d, &wd, &hh, &mm, &ss);
	if (want_date) {
		outs("DATE: ");
		outs(wdn[wd]);
		outc(' ');
		outnum((long) y);
		outc('-');
		out2(m);
		outc('-');
		out2(d);
		outs(" (UTC)\n");
	} else {
		outs("TIME: ");
		out2(hh);
		outc(':');
		out2(mm);
		outc(':');
		out2(ss);
		outs(" UTC (");
		outnum((long) t);
		outs(" s since 1970)\n");
	}
}

/* An unknown command word, or a line that starts with a quoted name: run it as a program in the foreground (the RUN
 * path, so the same kernel search, arguments and messages), waiting for its exit status. */
static char r13b_runline[SH_LINE_MAX + 2];

static void r13b_run_word(const char *word, const char *arg)
{
	unsigned int k = 0, i;

	for (i = 0; word[i] && k < SH_LINE_MAX; i++)
		r13b_runline[k++] = word[i];
	if (*arg && k < SH_LINE_MAX)
		r13b_runline[k++] = ' ';
	for (i = 0; arg[i] && k < SH_LINE_MAX; i++)
		r13b_runline[k++] = arg[i];
	r13b_runline[k] = 0;
	run_direct = 1;
	cmd_run(r13b_runline);
	run_direct = 0;
}
