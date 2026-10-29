/* R12C shell (-DR12C_SHELL): TASKS (the kernel task selector, or a list) and SYNC.  Thin clients of QT_TASKMGR,
 * QT_JOB_QUERY v2 and QT_SYNC; see docs/R12C-TASK-MANAGER-ABI.md and docs/R12C-SYNC-DURABILITY.md.  Included by
 * shell.c after shell_env.inc. */

#define APP_QT_TASKMGR   0x3Au
#define APP_QT_SYNC      0x39u
#define TQ_HDR 16u
#define TQ_ENT 128u
static unsigned char tqbuf[TQ_HDR + TQ_ENT * 16u];
static unsigned int syncst[16];

static void out_w(const char *s, int w)
{
	int n = (int) slen(s);

	outs(s);
	while (n++ < w)
		outc(' ');
}

static void out_numw2(long v, int w)
{
	char t[12];
	int n = 0, k;
	unsigned long u = (unsigned long) (v < 0 ? -v : v);

	do {
		t[n++] = (char) ('0' + u % 10u);
		u /= 10u;
	} while (u && n < 11);
	k = n + (v < 0);
	if (v < 0)
		outc('-');
	while (n)
		outc(t[--n]);
	while (k++ < w)
		outc(' ');
}

static unsigned int rd32c(const unsigned char *p)
{
	return (unsigned int) p[0] | ((unsigned int) p[1] << 8) | ((unsigned int) p[2] << 16) | ((unsigned int) p[3] << 24);
}

static const char *tq_state(unsigned int s)
{
	switch (s) {
	case 1: return "RUN";
	case 2: return "HELD";
	case 3: return "SLEEP";
	case 4: return "ENDED";
	case 5: return "INPUT";
	default: return "?";
	}
}

/* TASKS        open the task selector (Ctrl+Esc does the same; Esc returns)
 * TASKS /L     list every job: id, console (+ shared), state, parent, flags, program, title */
static void cmd_tasks(char *arg)
{
	int n, i;

	fold(arg);
	if (*arg == 0) {
		n = (int) qtrap(APP_QT_TASKMGR, 0, 0, 0);
		if (n < 0)
			outs("no task selector in this kernel (code -1)\n");
		return;
	}
	if (!same(arg, "/L")) {
		outs("usage: TASKS [/L]\n");
		return;
	}
	n = (int) qtrap(APP_QT_JOB_QUERY, (unsigned int) tqbuf, sizeof tqbuf, 2u);
	if (n < 0) {
		outs("task list not available (code ");
		outnum(n);
		outs(")\n");
		return;
	}
#ifdef R13_SHELL
	outs("ID        CON MODE STATE  PARENT    FL PROGRAM          TITLE\n");
#else
	outs("ID        CON STATE  PARENT    FL PROGRAM          TITLE\n");
#endif
	for (i = 0; i < n; i++) {
		const unsigned char *e = tqbuf + TQ_HDR + (unsigned int) i * TQ_ENT;
		char cb[4];
		unsigned int fl = e[8];

		out_numw2((long) rd32c(e), 10);
#ifdef R13_SHELL
		{
			char c3[4];
			int k = 0;

			if (e[7] >= 10)
				c3[k++] = (char) ('0' + e[7] / 10);
			c3[k++] = (char) ('0' + e[7] % 10);
			c3[k++] = (char) (e[9] > 1 ? '+' : ' ');
			c3[k] = 0;
			out_w(c3, 4);
			out_w(sess_kind(e[7]) == 2 ? "GFX" : "TEXT", 5);
		}
#else
		cb[0] = (char) ('0' + (e[7] & 7));
		cb[1] = (char) (e[9] > 1 ? '+' : ' ');
		cb[2] = 0;
		out_w(cb, 4);
#endif
		out_w(tq_state(e[5]), 7);
		if (rd32c(e + 12))
			out_numw2((long) rd32c(e + 12), 10);
		else
			out_w("-", 10);
		cb[0] = (char) ((fl & 0x10u) ? 'K' : (fl & 0x08u) ? 'P' : '-');
		cb[1] = (char) (e[10] == 2 ? 'R' : e[10] == 1 ? 'C' : '-');
		cb[2] = 0;
		out_w(cb, 3);
		out_w((const char *) e + 24, 17);
		outs((const char *) e + 56);
		outc('\n');
	}
	outnum(n);
	outs(" job(s).  CON +: console shared by several programs\n");
	outs("FL: K kernel job, P protected; title: C compiled in, R set at run time, - name\n");
}

static void sync_status(void)
{
	if (qtrap(APP_QT_SYNC, 1u, (unsigned int) syncst, 0) < 0) {
		outs("SYNC status not available\n");
		return;
	}
	outs("SYNC: ");
	outnum((long) syncst[1]);
	outs(" syncs, ");
	outnum((long) syncst[2]);
	outs(" ok, ");
	outnum((long) syncst[3]);
	outs(" failed; last result ");
	outnum((long) (int) syncst[4]);
	outs(" at tick ");
	outnum((long) syncst[5]);
	outs("\n      periodic ");
	outnum((long) syncst[8]);
	outs(" (failed ");
	outnum((long) syncst[9]);
	outs("), interval ");
	outnum((long) syncst[10]);
	outs(" s\n      device flushes ");
	outnum((long) syncst[12]);
	outs(", errors ");
	outnum((long) syncst[13]);
	outs(", timeouts ");
	outnum((long) syncst[14]);
	outc('\n');
}

static int set_interval(const char *v)
{
	long s = 0;
	int rc;

	if (*v == 0)
		return -1;
	while (*v) {
		if (*v < '0' || *v > '9' || s > 100000L)
			return -1;
		s = s * 10 + (*v++ - '0');
	}
	rc = (int) qtrap(APP_QT_SYNC, 2u, (unsigned int) s, 0);
	if (rc < 0)
		return rc;
	if (s == 0)
		outs("SYNC: periodic sync off\n");
	else {
		outs("SYNC: periodic sync every ");
		outnum(s);
		outs(" s\n");
	}
	return 0;
}

/* SYNC              write pending directory entries, flush the drive's cache; report the outcome
 * SYNC /STATUS      the counters
 * SYNC /INTERVAL:n  periodic sync by SYSTASK every n seconds (0 off; SET SYNC_INTERVAL=n does the same) */
static void cmd_sync(char *arg)
{
	int rc;

	fold(arg);
	if (same(arg, "/STATUS")) {
		sync_status();
		return;
	}
	if (arg[0] == '/' && arg[1] == 'I' && arg[2] == 'N' && arg[3] == 'T' && arg[4] == 'E' && arg[5] == 'R' &&
	    arg[6] == 'V' && arg[7] == 'A' && arg[8] == 'L' && arg[9] == ':') {
		if (set_interval(arg + 10) != 0)
			outs("usage: SYNC /INTERVAL:seconds (0..3600)\n");
		return;
	}
	if (*arg != 0) {
		outs("usage: SYNC [/STATUS | /INTERVAL:seconds]\n");
		return;
	}
	rc = (int) qtrap(APP_QT_SYNC, 0, 0, 0);
	(void) qtrap(APP_QT_SYNC, 1u, (unsigned int) syncst, 0);
	if (rc == 0) {
		outs("SYNC: done (");
		outnum((long) syncst[6]);
		outs(" open file entries written, drive cache flushed)\n");
		return;
	}
	outs("SYNC: FAILED (code ");
	outnum(rc);
	outs(") - ");
	switch (rc) {
	case -96:
		outs("FLUSH CACHE error (error register ");
		outnum((long) syncst[7]);
		outs(syncst[7] == 4u ? ": aborted)\n" : ")\n");
		outs("      the data may not be on stable storage\n");
		break;
	case -97:
		outs("the drive did not complete FLUSH CACHE in time\n");
		break;
	case -18:
		outs("no volume\n");
		break;
	case -1:
		outs("no SYNC service in this kernel\n");
		break;
	default:
		outs("a directory entry could not be written\n");
	}
}

/* SET SYNC_INTERVAL=n is forwarded to the kernel (and kept in the environment as usual). */
static void sync_interval_hook(const char *name, const char *value)
{
	char up[16];
	unsigned int i;

	for (i = 0; name[i] && i < sizeof up - 1; i++)
		up[i] = (char) ((name[i] >= 'a' && name[i] <= 'z') ? name[i] - 32 : name[i]);
	up[i] = 0;
	if (!same(up, "SYNC_INTERVAL"))
		return;
	if (value == 0)
		value = "0";
	if (set_interval(value) != 0)
		outs("SYNC_INTERVAL: not a number of seconds 0..3600; the kernel setting is unchanged\n");
}
