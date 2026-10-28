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
#define CON_COUNT  4

struct sh_snap {                           /* struct chan_con_snap of the kernel */
	unsigned char con, row, pad[2];
	unsigned char text[84];
};
static struct sh_snap snaprq;

static int sh_ctl(unsigned int op, unsigned int arg)
{
	return (int) qtrap(APP_QT_CH_CTL, (unsigned int) con_ch, op, arg);
}

/* One decimal digit argument 0..9, or -1. */
static int digit_arg(const char *a)
{
	if (a[0] >= '0' && a[0] <= '9' && a[1] == 0)
		return a[0] - '0';
	return -1;
}

static void cmd_con(char *arg)
{
	int n, v;

	if (*arg == 0) {
		v = sh_ctl(CC_QUERY, CVQ_SELF);
		outs("this shell is on console ");
		outnum(v & 0xff);
		outs(", console ");
		outnum(sh_ctl(CC_QUERY, CVQ_FG));
		outs(" is visible\n");
		return;
	}
	n = digit_arg(arg);
	if (n < 0 || n >= CON_COUNT) {
		outs("usage: CON [0-3]\n");
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
		outs("usage: CONDUMP [0-3]\n");
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
	run_bg = 1;
	run_con = con;
	cmd_run(arg);
	run_bg = 0;
	run_con = -1;
	if (con >= 0)
		sh_ctl(CC_CHILD, 0xffu);
}

/* ---- PS, WAIT, KILL (kernels built with -DQDOS_JOBCTL; docs/R11M-JOB-CONTROL-ABI.md) ---- */
#define JC_HDR     16u
#define JC_ENT     40u
#define JC_VER     1u
#define JC_MAXJOBS 8
static unsigned char jbuf[JC_HDR + JC_ENT * JC_MAXJOBS];

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

static void cmd_wait(char *arg)
{
	unsigned long id = parse_id(arg);
	int st, n;

	if (id == 0) {
		outs("usage: WAIT <job id>   (ids are shown by START and PS)\n");
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

static void cmd_kill(char *arg)
{
	unsigned long id = parse_id(arg);
	int r;

	if (id == 0) {
		outs("usage: KILL <job id>\n");
		return;
	}
	r = (int) qtrap(APP_QT_JOB_KILL, (unsigned int) id, 0, 0);
	if (r == -1) {
		outs("KILL: job control is not available in this kernel\n");
		return;
	}
	if (r == -2) {
		outs("no such job (unknown or stale id)\n");
		return;
	}
	if (r == -67) {
		outs("refused: not a started job\n");
		return;
	}
	if (r == -68) {
		outs("refused: a job cannot kill itself\n");
		return;
	}
	if (r == -69) {
		outs("job has already ended\n");
		return;
	}
	if (r < 0) {
		outs("KILL failed, error ");
		outnum(r);
		outc('\n');
		return;
	}
	while (qtrap(APP_QT_JOB_RECLAIMED, (unsigned int) id, 0, 0) == 0)
		qtrap(APP_QT_SUSPEND, 1, 0, 0);
	outs("job ");
	outnum((long) id);
	outs(" killed\n");
}
