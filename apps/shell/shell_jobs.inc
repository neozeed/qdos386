/*
 * shell_jobs.inc - R11M: console and background-start commands of the shell (-DR11M_JOBS).  Included by shell.c after
 * the directory commands.  Stage 2 provides CON, CONDUMP, DIAG, SLEEP and a minimal START; the job table commands
 * (PS, WAIT, KILL) follow in Stage 3.
 */
#define CC_QUERY   0x13u
#define CC_FOCUS   0x17u
#define CC_CHILD   0x18u
#define CC_SNAP    0x19u
#define CC_DIAG    0x1Au
#define CVQ_SELF   3u
#define CVQ_FG     6u
#ifdef R13_SHELL
#define CON_COUNT  20                      /* R13: sessions 0..19 (0..3 the legacy consoles, Alt+F1..F4) */
#define APP_QT_SURFACE 0x3Bu
#define SURF_LIST  17u
static unsigned int sbuf[4 + 8 * 20];      /* QT_SURFACE list: header + 20 records */
#else
#define CON_COUNT  4
#endif

struct sh_snap {                           /* struct chan_con_snap of the kernel */
	unsigned char con, row, pad[2];
	unsigned char text[84];
};
static struct sh_snap snaprq;

static int sh_ctl(unsigned int op, unsigned int arg)
{
	return (int) qtrap(APP_QT_CH_CTL, (unsigned int) con_ch, op, arg);
}

#ifdef R13_SHELL
/* R13: a decimal argument 0..99, or -1. */
static int digit_arg(const char *a)
{
	if (a[0] >= '0' && a[0] <= '9' && a[1] == 0)
		return a[0] - '0';
	if (a[0] >= '0' && a[0] <= '9' && a[1] >= '0' && a[1] <= '9' && a[2] == 0)
		return (a[0] - '0') * 10 + (a[1] - '0');
	return -1;
}

/* The kernel's session list into sbuf -> the number of records.  A kernel without QT_SURFACE (R12C and older) has
 * the four consoles only: they are listed as TEXT sessions 0..3. */
static int sess_list(void)
{
	int c = (int) qtrap(APP_QT_SURFACE, SURF_LIST, (unsigned int) sbuf, sizeof sbuf), k;

	if (c >= 0)
		return c;
	for (k = 0; k < 4; k++) {
		int f;
		for (f = 0; f < 8; f++)
			sbuf[4 + k * 8 + f] = 0;
		sbuf[4 + k * 8 + 1] = (unsigned int) k;
		sbuf[4 + k * 8 + 2] = 1u;
	}
	return 4;
}

/* The kind (1 TEXT, 2 INDEX8) of session n from the kernel's session list, 0 if there is none. */
static int sess_kind(int n)
{
	int k, c = sess_list();

	for (k = 0; k < c; k++)
		if ((int) sbuf[4 + k * 8 + 1] == n)
			return (int) sbuf[4 + k * 8 + 2];
	return 0;
}
#else
/* One decimal digit argument 0..9, or -1. */
static int digit_arg(const char *a)
{
	if (a[0] >= '0' && a[0] <= '9' && a[1] == 0)
		return a[0] - '0';
	return -1;
}
#endif

static int jobs_query(void);
static unsigned int rd32(const unsigned char *p);
static void out_pad(const char *s, int w);

#ifdef R13_SHELL
/* R13: CON with no argument: every session (the legacy consoles and the private sessions), its mode and its jobs. */
static void con_table(void)
{
	int c, i, k, n, vis, me, any, ns, s;
	extern unsigned char jbuf[];

	me = sh_ctl(CC_QUERY, CVQ_SELF) & 0xff;
	vis = sh_ctl(CC_QUERY, CVQ_FG);
	outs("this shell is on session ");
	outnum(me);
	outs(", session ");
	outnum(vis);
	outs(" is shown\n");
	ns = sess_list();
	n = jobs_query();
	if (n < 0 || ns < 0)
		return;
	for (s = 0; s < ns; s++) {
		const unsigned int *se = sbuf + 4 + s * 8;

		c = (int) se[1];
		outs(c == vis ? "* " : "  ");
		outs(c < 10 ? "session  " : "session ");
		outnum(c);
		if (c < 4) {
			outs("  Alt+F");
			outnum(c + 1);
		} else {
			outs("  private");
		}
		outs(se[2] == 2 ? "  GFX " : "  TEXT");
		if (se[2] == 2) {
			outnum((long) se[3]);
			outc('x');
			outnum((long) se[4]);
		}
		outs("  ");
		any = 0;
		for (i = 0; i < n; i++) {
			const unsigned char *e = jbuf + 16u + (unsigned int) i * 40u;
			char nm[17];

			if (e[7] != (unsigned int) c || !(e[8] & 5))
				continue;
			for (k = 0; k < 16; k++)
				nm[k] = (char) e[24 + k];
			nm[16] = 0;
			if (any)
				outs(", ");
			any = 1;
			outs(nm);
			if (e[8] & 4)
				outs(" (this shell)");
			else {
				outs(" [");
				outnum((long) rd32(e));
				outc(']');
			}
		}
		if (!any)
			outs("(no job)");
		outc('\n');
	}
}
#else
/* CON with no argument: where this shell is, which console is visible, and the jobs on each console. */
static void con_table(void)
{
	int c, i, k, n, vis, me, any;
	extern unsigned char jbuf[];

	me = sh_ctl(CC_QUERY, CVQ_SELF) & 0xff;
	vis = sh_ctl(CC_QUERY, CVQ_FG);
	outs("this shell is on console ");
	outnum(me);
	outs(", console ");
	outnum(vis);
	outs(" is visible\n");
	n = jobs_query();
	if (n < 0)
		return;
	for (c = 0; c < CON_COUNT; c++) {
		outs(c == vis ? "* " : "  ");
		outs("console ");
		outnum(c);
		outs("  Alt+F");
		outnum(c + 1);
		outs("  ");
		any = 0;
		for (i = 0; i < n; i++) {
			const unsigned char *e = jbuf + 16u + (unsigned int) i * 40u;
			char nm[17];

			if (e[7] != (unsigned int) c || !(e[8] & 5))      /* only started jobs and this shell (not IDLE) */
				continue;
			for (k = 0; k < 16; k++)
				nm[k] = (char) e[24 + k];
			nm[16] = 0;
			if (any)
				outs(", ");
			any = 1;
			outs(nm);
			if (e[8] & 4)
				outs(" (this shell)");
			else {
				outs(" [");
				outnum((long) rd32(e));
				outc(']');
			}
		}
		if (!any)
			outs("(no job)");
		outc('\n');
	}
}
#endif

static void cmd_con(char *arg)
{
	int n, v;

	if (*arg == 0) {
		con_table();
		return;
	}
	n = digit_arg(arg);
	if (n < 0 || n >= CON_COUNT) {
#ifdef R13_SHELL
		outs("usage: CON [n]   (n = session 0..19; no argument: show the sessions and their jobs)\n");
#else
		outs("usage: CON [0-3]   (no argument: show the consoles and their jobs)\n");
#endif
		return;
	}
	v = sh_ctl(CC_FOCUS, (unsigned int) n);
	if (v < 0) {
		outs("no such console\n");
		return;
	}
}

static void cmd_condump(char *arg)
{
	int n = digit_arg(arg), r, last = -1, row;

	if (n < 0 || n >= CON_COUNT) {
#ifdef R13_SHELL
		outs("usage: CONDUMP n   (session 0..19)\n");
#else
		outs("usage: CONDUMP [0-3]\n");
#endif
		return;
	}
	for (row = 0; row < 25; row++) {
		snaprq.con = (unsigned char) n;
		snaprq.row = (unsigned char) row;
		r = sh_ctl(CC_SNAP, (unsigned int) &snaprq);
		if (r > 0)
			last = row;
	}
	outs("CONDUMP ");
	outnum(n);
	outs(":\n");
	for (row = 0; row <= last; row++) {
		snaprq.con = (unsigned char) n;
		snaprq.row = (unsigned char) row;
		r = sh_ctl(CC_SNAP, (unsigned int) &snaprq);
		outs("|");
		if (r > 0) {
			snaprq.text[r] = 0;
			out((const char *) snaprq.text, (unsigned int) r);
		}
		outc('\n');
	}
	outs("CONDUMP END\n");
}

static void cmd_diag(char *arg)
{
	int n = digit_arg(arg);

	if (*arg == 0) {
		outs("diagnostic mode ");
		outnum(sh_ctl(CC_DIAG, 0xffu));
		outc('\n');
		return;
	}
	if (n < 0 || n > 2) {
		outs("usage: DIAG 0|1|2\n");
		return;
	}
	sh_ctl(CC_DIAG, (unsigned int) n);
}

static void cmd_sleep(char *arg)
{
	unsigned long t = 0;
	unsigned int i;

	if (*arg == 0) {
		outs("usage: SLEEP <ticks>\n");
		return;
	}
	for (i = 0; arg[i] >= '0' && arg[i] <= '9' && t < 100000; i++)
		t = t * 10 + (unsigned long) (arg[i] - '0');
	if (arg[i] != 0 || t == 0) {
		outs("usage: SLEEP <ticks>\n");
		return;
	}
	qtrap(APP_QT_SUSPEND, (unsigned int) t, 0, 0);
}

/* START path [args] [/CON:n]: like RUN, but the shell does not wait.  The /CON:n word is removed from the argument text. */
static void cmd_start(char *arg)
{
	char *p;
	int con = -1;

#ifdef R13_SHELL
	run_new = 0;
	for (p = arg; *p; p++) {          /* R13: /NEW (a private session) and /CON:n with n up to 19 */
		int nd = 0, v = 0;

		if (!(p == arg || p[-1] == ' ') || p[0] != '/')
			continue;
		if ((p[1] == 'N' || p[1] == 'n') && (p[2] == 'E' || p[2] == 'e') && (p[3] == 'W' || p[3] == 'w') &&
		    (p[4] == 0 || p[4] == ' ')) {
			char *q = p + 4;

			run_new = 1;
			while (*q == ' ')
				q++;
			while ((*p++ = *q++) != 0)
				;
			break;
		}
		if (!((p[1] == 'C' || p[1] == 'c') && (p[2] == 'O' || p[2] == 'o') && (p[3] == 'N' || p[3] == 'n') && p[4] == ':'))
			continue;
		while (p[5 + nd] >= '0' && p[5 + nd] <= '9' && nd < 3)
			v = v * 10 + (p[5 + nd++] - '0');
		if (nd == 0 || nd > 2 || !(p[5 + nd] == 0 || p[5 + nd] == ' '))
			continue;
		if (v >= CON_COUNT || sess_kind(v) == 0) {
			outs("no such session\n");
			return;
		}
		con = v;
		{
			char *q = p + 5 + nd;

			while (*q == ' ')
				q++;
			while ((*p++ = *q++) != 0)
				;
		}
		break;
	}
	if (0)
#endif
	for (p = arg; *p; p++) {
		if ((p == arg || p[-1] == ' ') && p[0] == '/' && (p[1] == 'C' || p[1] == 'c') && (p[2] == 'O' || p[2] == 'o') &&
		    (p[3] == 'N' || p[3] == 'n') && p[4] == ':' && p[5] >= '0' && p[5] <= '9' && (p[6] == 0 || p[6] == ' ')) {
			char *q = p + 6;

			con = p[5] - '0';
			if (con >= CON_COUNT) {
				outs("no such console\n");
				return;
			}
			while (*q == ' ')
				q++;
			while ((*p++ = *q++) != 0)
				;
			break;
		}
	}
	p = arg;
	if (*p == 0) {
		outs("usage: START <name> [arguments] [/CON:n]\n");
		return;
	}
	if (con >= 0 && sh_ctl(CC_CHILD, (unsigned int) con) < 0) {
		outs("cannot select console\n");
		return;
	}
#ifdef R13_SHELL
	if (run_new && sh_ctl(CC_CHILD, 0xfeu) < 0) {
		outs("this kernel has no private sessions\n");
		run_new = 0;
		return;
	}
#endif
	run_bg = 1;
	run_con = con;
	cmd_run(arg);
	run_bg = 0;
	run_con = -1;
	if (con >= 0)
		sh_ctl(CC_CHILD, 0xffu);
#ifdef R13_SHELL
	if (run_new)
		sh_ctl(CC_CHILD, 0xffu);
	run_new = 0;
#endif
}

/* ---- PS, WAIT, KILL (kernels built with -DQDOS_JOBCTL; docs/R11M-JOB-CONTROL-ABI.md) ---- */
#define JC_HDR     16u
#define JC_ENT     40u
#define JC_VER     1u
#ifndef JC_MAXJOBS
#define JC_MAXJOBS 8
#endif
unsigned char jbuf[JC_HDR + JC_ENT * JC_MAXJOBS];

static unsigned int rd32(const unsigned char *p)
{
	return (unsigned int) p[0] | ((unsigned int) p[1] << 8) | ((unsigned int) p[2] << 16) | ((unsigned int) p[3] << 24);
}

static void out_pad(const char *s, int w)
{
	int n = (int) slen(s);

	outs(s);
	while (n++ < w)
		outc(' ');
}

static void out_numw(long v, int w)
{
	int n = 1;
	long t = v < 0 ? -v : v;

	if (v < 0)
		n++;
	while (t >= 10) {
		t /= 10;
		n++;
	}
	outnum(v);
	while (n++ < w)
		outc(' ');
}

/* Read the kernel's job records into jbuf.  Returns the count, or a negative code. */
static int jobs_query(void)
{
	return (int) qtrap(APP_QT_JOB_QUERY, (unsigned int) jbuf, sizeof jbuf, JC_VER);
}

/* The record of a job id, or 0. */
static const unsigned char *jobs_find(int n, unsigned int id)
{
	int i;

	for (i = 0; i < n; i++)
		if (rd32(jbuf + JC_HDR + (unsigned int) i * JC_ENT) == id)
			return jbuf + JC_HDR + (unsigned int) i * JC_ENT;
	return 0;
}

static const char *state_name(unsigned int s)
{
	switch (s) {
	case 1: return "RUN";
	case 2: return "HELD";
	case 3: return "SLEEP";
	case 4: return "DEAD";
	case 5: return "CONWAIT";
	}
	return "?";
}

static void say_end(int id, int st)
{
	outs("job ");
	outnum(id);
	if (st == -66)
		outs(" was killed\n");
	else if (st == E_NOSTATUS)
		outs(" ended without an exit status\n");
	else {
		outs(" exit status ");
		outnum(st);
		outc('\n');
	}
}

static void cmd_ps(void)
{
	int n = jobs_query(), i, k;
	unsigned int cnt;

	if (n < 0) {
		outs("PS: job control is not available in this kernel\n");
		return;
	}
	cnt = (unsigned int) n;
	outs("ID        SLOT STATE   PRI CON AGE     NAME\n");
	for (i = 0; i < (int) cnt; i++) {
		const unsigned char *e = jbuf + JC_HDR + (unsigned int) i * JC_ENT;
		char nm[17];

		for (k = 0; k < 16; k++)
			nm[k] = (char) e[24 + k];
		nm[16] = 0;
		out_numw((long) rd32(e), 9);
		outc(' ');
		out_numw((long) e[4], 4);
		outc(' ');
		out_pad(state_name(e[5]), 8);
		out_numw((long) e[6], 4);
		if (e[7] == 0xff)
			out_pad("-", 4);
		else
			out_numw((long) e[7], 4);
		out_numw((long) rd32(e + 16), 8);
		outs(nm);
		if (e[8] & 4)
			outs("  (this shell)");
		else if (e[8] & 2)
			outs("  (paged)");
		outc('\n');
	}
	jt_harvest();
	for (i = 0; i < jt_n; i++) {                 /* started jobs that are gone: their recorded end */
		int st;

		if (jobs_find(n, (unsigned int) jt[i].id))
			continue;
		st = jt[i].done ? jt[i].status : (int) qtrap(APP_QT_JOB_EXIT, (unsigned int) jt[i].id, 0, 0);
		out_numw((long) jt[i].id, 9);
		outs(" -    ENDED   -   ");
		if (jt[i].con >= 0)
			out_numw((long) jt[i].con, 4);
		else
			out_pad("", 4);
		out_pad("", 8);
		outs(jt[i].name);
		if (st == -2)
			outs("  (slot reused: status lost)");
		else if (st == -66)
			outs("  (killed)");
		else if (st == E_NOSTATUS)
			outs("  (no exit status)");
		else {
			outs("  (status ");
			outnum(st);
			outc(')');
		}
		outc('\n');
	}
	outnum(n);
	outs(" job(s) in the kernel table\n");
}

/* A job id typed as a decimal number.  0 on a bad argument. */
static unsigned long parse_id(const char *a)
{
	unsigned long v = 0;
	int i;

	if (*a == 0)
		return 0;
	for (i = 0; a[i]; i++) {
		if (a[i] < '0' || a[i] > '9' || i >= 10)
			return 0;
		v = v * 10 + (unsigned long) (a[i] - '0');
		if (v > 0x7fffffffUL)
			return 0;
	}
	return v;
}

/* While the shell waits for a job, keys typed on the shell's console are read and dropped; Ctrl+C abandons the wait.  Returns 1 for Ctrl+C. */
static int wait_cancelled(void)
{
	int r, hit = 0;

	for (;;) {
		r = qtrap(APP_QT_CH_READ, (unsigned int) con_ch, (unsigned int) rbuf, 1);
		if (r != 1)
			return hit;
		if (rbuf[0] == 3)
			hit = 1;
	}
}

static void wait_abandoned(unsigned long id)
{
	outs("WAIT abandoned (Ctrl+C); job ");
	outnum((long) id);
	outs(" keeps running\n");
}

static void cmd_wait(char *arg)
{
	unsigned long id = parse_id(arg);
	int st, n;

	if (id == 0) {
		outs("usage: WAIT <job id>   (ids are shown by START and PS; Ctrl+C stops waiting)\n");
		return;
	}
	jt_harvest();
	for (n = 0; n < jt_n; n++)
		if (jt[n].id == (int) id && jt[n].done && jt[n].status != -2) {
			say_end((int) id, jt[n].status);   /* ended earlier: the status the shell recorded */
			return;
		}
	st = (int) qtrap(APP_QT_JOB_EXIT, (unsigned int) id, 0, 0);
	if (st == -2) {
		outs("no such job (unknown or stale id)\n");
		return;
	}
	if (st == E_RUNNING) {
		const unsigned char *e;

		n = jobs_query();
		e = n >= 0 ? jobs_find(n, (unsigned int) id) : 0;
		if (e == 0 || !(e[8] & 1) || (e[8] & 4)) {
			outs("not a started job (WAIT only waits for jobs started with START or RUN)\n");
			return;
		}
		for (;;) {
			st = (int) qtrap(APP_QT_JOB_EXIT, (unsigned int) id, 0, 0);
			if (st != E_RUNNING)
				break;
			if (wait_cancelled()) {
				wait_abandoned(id);
				return;
			}
			qtrap(APP_QT_SUSPEND, 1, 0, 0);
		}
	}
	while (qtrap(APP_QT_JOB_RECLAIMED, (unsigned int) id, 0, 0) == 0)
		qtrap(APP_QT_SUSPEND, 1, 0, 0);
	for (n = 0; n < jt_n; n++)
		if (jt[n].id == (int) id && !jt[n].done) {
			jt[n].done = 1;
			jt[n].status = st;
		}
	say_end((int) id, st);
}

/* Kill one job; prints the outcome.  Returns 1 if it was killed. */
static int kill_one(unsigned long id, int quiet)
{
	int r = (int) qtrap(APP_QT_JOB_KILL, (unsigned int) id, 0, 0);

	if (r == -1) {
		outs("KILL: job control is not available in this kernel\n");
		return 0;
	}
	if (r == -2) {
		outs("no such job (unknown or stale id)\n");
		return 0;
	}
	if (r == -67) {
		outs("refused: not a started job\n");
		return 0;
	}
	if (r == -68) {
		outs("refused: a job cannot kill itself\n");
		return 0;
	}
	if (r == -69) {
		outs("job has already ended\n");
		return 0;
	}
	if (r < 0) {
		outs("KILL failed, error ");
		outnum(r);
		outc('\n');
		return 0;
	}
	while (qtrap(APP_QT_JOB_RECLAIMED, (unsigned int) id, 0, 0) == 0)
		qtrap(APP_QT_SUSPEND, 1, 0, 0);
	if (!quiet) {
		outs("job ");
		outnum((long) id);
		outs(" killed\n");
	}
	return 1;
}

/* KILL /CON:n: end every started job on console n (the shell itself is never one). */
static void kill_console(int con)
{
	unsigned long ids[JC_MAXJOBS];
	int n = jobs_query(), i, cnt = 0, done = 0;

	if (n < 0) {
		outs("KILL: job control is not available in this kernel\n");
		return;
	}
	for (i = 0; i < n && cnt < JC_MAXJOBS; i++) {
		const unsigned char *e = jbuf + JC_HDR + (unsigned int) i * JC_ENT;

		if (e[7] == (unsigned int) con && (e[8] & 1) && !(e[8] & 4))
			ids[cnt++] = rd32(e);
	}
	if (cnt == 0) {
		outs("no started job on console ");
		outnum(con);
		outc('\n');
		return;
	}
	for (i = 0; i < cnt; i++)
		done += kill_one(ids[i], 1);
	outnum(done);
	outs(" job(s) killed on console ");
	outnum(con);
	outc('\n');
}

static void cmd_kill(char *arg)
{
	unsigned long id;

#ifdef R13_SHELL
	if (arg[0] == '/' && (arg[1] == 'C' || arg[1] == 'c') && (arg[2] == 'O' || arg[2] == 'o') && (arg[3] == 'N' || arg[3] == 'n') &&
	    arg[4] == ':' && digit_arg(arg + 5) >= 0) {
		if (digit_arg(arg + 5) >= CON_COUNT) {
			outs("no such session\n");
			return;
		}
		kill_console(digit_arg(arg + 5));
		return;
	}
#endif
	if (arg[0] == '/' && (arg[1] == 'C' || arg[1] == 'c') && (arg[2] == 'O' || arg[2] == 'o') && (arg[3] == 'N' || arg[3] == 'n') && arg[4] == ':' &&
	    arg[5] >= '0' && arg[5] <= '9' && arg[6] == 0) {
		if (arg[5] - '0' >= CON_COUNT) {
			outs("no such console\n");
			return;
		}
		kill_console(arg[5] - '0');
		return;
	}
	id = parse_id(arg);
	if (id == 0) {
		outs("usage: KILL <job id> | KILL /CON:n   (ids are shown by START and PS)\n");
		return;
	}
	kill_one(id, 0);
}

/* EXIT ends the whole session (the kernel stops when the shell does), so started jobs would be lost with their state.  The first EXIT
 * with started jobs still running only warns; an immediate second EXIT leaves. */
static int exit_warned;

static int exit_blocked(void)
{
	int n = jobs_query(), i, live = 0;

	if (n < 0 || exit_warned)
		return 0;
#ifdef R13B_DYN
	/* R13B: a dynamic shell counts only the jobs it started itself (the other shells' jobs are not its business) */
	jt_harvest();
	for (i = 0; i < jt_n; i++)
		if (!jt[i].done)
			live++;
#else
	for (i = 0; i < n; i++) {
		const unsigned char *e = jbuf + JC_HDR + (unsigned int) i * JC_ENT;

		if ((e[8] & 1) && !(e[8] & 4) && e[5] != 4)
			live++;
	}
#endif
	if (live == 0)
		return 0;
	exit_warned = 1;
	outnum(live);
#ifdef R13B_DYN
	outs(" started job(s) still running; they go on after this shell ends (KILL or Ctrl+Esc F1 ends them later).\n");
	outs("Type EXIT again to leave this shell anyway.\n");
#else
	outs(" started job(s) still running; EXIT ends the whole session and stops them (PS lists them, KILL /CON:n ends them).\n");
	outs("Type EXIT again to leave anyway.\n");
#endif
	return 1;
}

#ifdef R13_SHELL
static int job_session(unsigned long id)
{
	int n = jobs_query(), i;

	for (i = 0; i < n; i++) {
		const unsigned char *e = jbuf + 16u + (unsigned int) i * 40u;

		if (rd32(e) == id)
			return (int) e[7];
	}
	return -1;
}
#endif
