/* NEGDYN.AOU (R12B Stage 5): the dynamic-process equivalents of the historical fixed-slot NEG.AOU / MULTI.AOU checks.
 *   usage: NEGDYN [cycles]       (default 1)
 * NEG.AOU (R11H) expected a second image at an occupied profile to be refused with -28 "slot busy", and MULTI.AOU expected
 * a collision on profile 2 to be refused.  Since R12A every program is a process with its own frames at its link origin,
 * so the invariant those checks protected - two programs never share memory - is now "several copies at one origin all
 * run, each with its own result", and the refusals that remain meaningful are the image checks and the id checks:
 *   1. the ten R11H malformed images (tests/r12a/add-neg-files.sh): each refused by qd_spawn with an image error
 *      (-20..-26), nothing left behind (frames, kernel heap, child records); a good pair still runs (3, 7);
 *   2. "contention": MULTIA twice and MULTIB twice at once (two copies per origin): all four run, exits 3 3 7 7;
 *   3. churn: 40 spawn/wait cycles; every old id is refused afterwards (not a child), never confused with a new one;
 *   4. invalid ids: 0, -1, a huge id, the caller, its parent: wait/poll/kill refused;
 *   5. after everything: frame pool, kernel heap and child records as at the start.
 * Prints "NEGDYN ok/FAIL ..." per check and "NEGDYN PASS checks=N" at the end (status 0) or FAIL (3). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qdos386.h"

static int checks, fails, logch = -1;

/* Every check goes to WIN1:/NEGDYN.LOG (the screen scrolls); the screen gets failures and the summaries. */
static void check(const char *what, int ok, long got)
{
	char line[200];

	checks++;
	if (!ok)
		fails++;
	sprintf(line, "NEGDYN %s %s (%ld)\n", ok ? "ok  " : "FAIL", what, got);
	if (logch >= 0)
		qd_write_all(logch, line, (unsigned int) strlen(line));
	if (!ok)
		printf("%s", line);
}

struct bad {
	const char *name;
	int r11h;                  /* the R11H code, for the record */
};

static const struct bad bads[] = {
	{ "BADMAG.AOU", -21 }, { "RELOC.AOU", -22 }, { "BADENT.AOU", -25 }, { "ENTOFF.AOU", -25 }, { "BADSYM.AOU", -23 },
	{ "BIGTXT.AOU", -24 }, { "TRUNC.AOU", -20 }, { "TRAIL.AOU", -20 }, { "SHORT.AOU", -20 }, { "EMPTYX.AOU", -20 }
};

static int spawn0(const char *name)
{
	return qd_spawn(name, "", 0, 0, QD_SPAWN_CON_OWN);
}

static void good_pair(const char *tag)
{
	char t[80];
	int a = spawn0("MULTIA.AOU"), b = spawn0("MULTIB.AOU"), sa, sb;

	sa = a > 0 ? qd_wait(a, 3000) : a;
	sb = b > 0 ? qd_wait(b, 3000) : b;
	sprintf(t, "good pair after %s: exits 3 and 7 (got %d %d)", tag, sa, sb);
	check(t, sa == 3 && sb == 7, sa * 100 + sb);
}

int main(int argc, char **argv)
{
	int cycles = argc > 1 ? atoi(argv[1]) : 1, k, i, rc, ids[4], st[4], old[40], n, bad_old;
	unsigned long heap0, h;
	struct qd_procinfo p0, p1, p;
	char t[120];

	logch = qd_open_ex("WIN1:/NEGDYN.LOG", QD_O_WRITE | QD_O_CREATE | QD_O_TRUNC | QD_O_FILE);
	printf("NEGDYN start: %d cycle(s), log WIN1:/NEGDYN.LOG\n", cycles);
	heap0 = qd_free_bytes();
	qd_proc_info(0, &p0);
	for (k = 0; k < cycles; k++) {
		/* 1. malformed images */
		for (i = 0; i < (int) (sizeof bads / sizeof bads[0]); i++) {
			h = qd_free_bytes();
			qd_proc_info(0, &p);
			rc = spawn0(bads[i].name);
			qd_proc_info(0, &p1);
			sprintf(t, "%s refused with an image error (R11H code %d)", bads[i].name, bads[i].r11h);
			check(t, rc <= -20 && rc >= -26, rc);
			sprintf(t, "%s left nothing behind (frames, heap, children)", bads[i].name);
			check(t, p1.pool_free == p.pool_free && qd_free_bytes() == h && p1.children == p.children,
			      (long) p1.pool_free - (long) p.pool_free);
		}
		good_pair("the malformed images");
		/* 2. two copies per origin at once */
		ids[0] = spawn0("MULTIA.AOU");
		ids[1] = spawn0("MULTIA.AOU");
		ids[2] = spawn0("MULTIB.AOU");
		ids[3] = spawn0("MULTIB.AOU");
		qd_proc_info(0, &p);
		check("two copies of MULTIA and two of MULTIB started together (no slot-busy refusal)",
		      ids[0] > 0 && ids[1] > 0 && ids[2] > 0 && ids[3] > 0, (long) p.children);
		for (i = 3; i >= 0; i--)
			st[i] = ids[i] > 0 ? qd_wait(ids[i], 3000) : ids[i];
		sprintf(t, "each copy ended with its own status (3 3 7 7: got %d %d %d %d)", st[0], st[1], st[2], st[3]);
		check(t, st[0] == 3 && st[1] == 3 && st[2] == 7 && st[3] == 7, 0);
		/* 3. churn */
		n = 0;
		for (i = 0; i < 40; i++) {
			old[i] = spawn0("MULTIB.AOU");
			if (old[i] > 0 && qd_wait(old[i], 3000) == 7)
				n++;
		}
		check("40 spawn/wait cycles, each status 7", n == 40, n);
		bad_old = 0;
		for (i = 0; i < 40; i++)
			if (qd_wait(old[i], 0) != QD_PROC_ERR_NOTCHILD || qd_poll(old[i]) != QD_PROC_ERR_NOTCHILD || qd_kill(old[i]) >= 0)
				bad_old++;
		check("every old id refused afterwards (wait, poll, kill)", bad_old == 0, bad_old);
		/* 4. invalid ids */
		check("wait(0) refused", qd_wait(0, 0) < 0, qd_wait(0, 0));
		check("wait(-1) refused", qd_wait(-1, 0) < 0, qd_wait(-1, 0));
		check("wait(huge id) refused", qd_wait(0x7FFFFFFF, 0) < 0, qd_wait(0x7FFFFFFF, 0));
		check("wait(self) refused", qd_wait(qd_self(), 0) < 0, qd_wait(qd_self(), 0));
		qd_proc_info(0, &p);
		check("wait(parent) refused", qd_wait((int) p.parent, 0) < 0, qd_wait((int) p.parent, 0));
		check("kill(parent) refused", qd_kill((int) p.parent) < 0, 0);
		check("poll(huge id) refused", qd_poll(0x7FFFFFFF) < 0 && qd_poll(0x7FFFFFFF) != QD_PROC_ERR_TIMEOUT, qd_poll(0x7FFFFFFF));
		printf("NEGDYN cycle %d done: checks %d fails %d\n", k + 1, checks, fails);
	}
	qd_proc_info(0, &p1);
	check("end: frame pool as at the start", p1.pool_free == p0.pool_free, (long) p1.pool_free - (long) p0.pool_free);
	check("end: kernel heap as at the start", qd_free_bytes() == heap0, (long) qd_free_bytes() - (long) heap0);
	check("end: no child records", p1.children == 0, (long) p1.children);
	{
		char line[80];

		sprintf(line, "NEGDYN %s checks=%d fails=%d\n", fails == 0 ? "PASS" : "FAIL", checks, fails);
		printf("%s", line);
		if (logch >= 0) {
			qd_write_all(logch, line, (unsigned int) strlen(line));
			qd_close(logch);
		}
	}
	return fails == 0 ? 0 : 3;
}
