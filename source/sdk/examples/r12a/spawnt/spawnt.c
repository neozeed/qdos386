/* SPAWNT.AOU (R12A Gate 3): process creation, wait, poll and kill from an ordinary application.
 *   usage: SPAWNT [cycles]          the full matrix, repeated `cycles` times (default 2) with a leak check
 *          SPAWNT orphan            start a long child and end without waiting for it
 *          SPAWNT inter             start CHILDT on console 3 that reads a line there; wait for it
 * Every check prints "SPAWNT ok ..." or "SPAWNT FAIL ..."; the end is "SPAWNT PASS checks=N" (status 0) or FAIL. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qdos386.h"

static int checks, fails, quiet, logch = -1;

/* Every check also goes to WIN1:/SPAWNT.LOG (the screen scrolls; the judge reads the file from the volume). */
static void logline(const char *s)
{
	unsigned int n = 0;

	while (s[n])
		n++;
	if (logch >= 0)
		qd_write_all(logch, s, n);
}

static void check(const char *what, int ok, long got)
{
	char line[160];

	checks++;
	sprintf(line, "SPAWNT %s %s (%ld)\n", ok ? "ok  " : "FAIL", what, got);
	logline(line);
	if (!ok) {
		fails++;
		printf("%s", line);
	} else if (!quiet) {
		printf("%s", line);
	}
}

/* packed argv[1..]: "a\0b\0c\0" */
static int spawn(const char *tag, int ticks, int status, unsigned int flags, int con)
{
	char buf[64];
	int n;

	n = sprintf(buf, "%s", tag) + 1;
	n += sprintf(buf + n, "%d", ticks) + 1;
	n += sprintf(buf + n, "%d", status) + 1;
	return qd_spawn("CHILDT.AOU", buf, (unsigned int) n, flags, con);
}

static void matrix(void)
{
	int a, b, c, d, rc, i, ids[10], self = qd_self(), nlim;
	struct qd_procinfo p0, p1;

	qd_proc_info(0, &p0);
	/* two children of the same image at the same origin; wait both, out of order */
	a = spawn("x", 40, 21, 0, QD_SPAWN_CON_OWN);
	b = spawn("y", 80, 22, 0, QD_SPAWN_CON_OWN);
	check("two children started (same image, same origin)", a > 0 && b > 0 && a != b, (long) b);
	{
		struct qd_procinfo ca, cb;
		int ia = qd_proc_info(a, &ca), ib = qd_proc_info(b, &cb);
		check("children have their own directories at origin 0x240000, parent = this job",
		      ia == 0 && ib == 0 && ca.space != cb.space && ca.space != p0.space && ca.origin == 0x240000ul &&
		      cb.origin == 0x240000ul && (int) ca.parent == self && (int) cb.parent == self, (long) ca.space);
	}
	check("poll of a running child: still running", qd_poll(b) == QD_PROC_ERR_TIMEOUT, (long) qd_poll(b));
	rc = qd_wait(b, -1);
	check("wait for the second child first: status 22", rc == 22, (long) rc);
	rc = qd_wait(a, -1);
	check("then the first: status 21", rc == 21, (long) rc);
	check("a reaped child is no longer a child", qd_wait(a, 0) == QD_PROC_ERR_NOTCHILD, (long) qd_wait(a, 0));
	/* wait with a timeout shorter than the child */
	c = spawn("t", 120, 23, 0, QD_SPAWN_CON_OWN);
	rc = qd_wait(c, 20);
	check("wait with a 20-tick timeout on a 120-tick child: timed out", rc == QD_PROC_ERR_TIMEOUT, (long) rc);
	rc = qd_wait(c, -1);
	check("then waiting forever returns its status 23", rc == 23, (long) rc);
	/* child that ends before the wait */
	c = spawn("e", 0, 24, 0, QD_SPAWN_CON_OWN);
	qd_yield(60);
	rc = qd_wait(c, 0);
	check("child ended before the wait: status kept (zombie) and returned by a poll", rc == 24, (long) rc);
	/* not children */
	check("wait for itself refused", qd_wait(self, 0) == QD_PROC_ERR_NOTCHILD, (long) qd_wait(self, 0));
	check("wait for id 0 refused", qd_wait(0, 0) == QD_PROC_ERR_NOTCHILD, 0);
	check("wait for an unknown id refused", qd_wait(0x7FFF0005, 0) == QD_PROC_ERR_NOTCHILD, 0);
	check("wait for the shell (not a child) refused", qd_wait(qd_job_slot(1), 0) == QD_PROC_ERR_NOTCHILD, 0);
	check("kill of a job that is not a child refused", qd_kill(qd_job_slot(1)) == QD_PROC_ERR_NOTCHILD, 0);
	/* kill a running child */
	c = spawn("k", 3000, 25, 0, QD_SPAWN_CON_OWN);
	qd_yield(10);
	rc = qd_kill(c);
	check("kill a running child", rc == 0, (long) rc);
	rc = qd_wait(c, -1);
	check("its wait reports 'killed' (-66)", rc == QD_ERR_KILLED, (long) rc);
	/* held child: cancel, and held then activated */
	c = spawn("h", 10, 26, QD_SPAWN_HELD, QD_SPAWN_CON_OWN);
	check("held child created", c > 0, (long) c);
	check("held child does not run (poll: still running)", qd_poll(c) == QD_PROC_ERR_TIMEOUT, (long) qd_poll(c));
	rc = qd_kill(c);
	check("held child cancelled before it ever ran", rc == 0 && qd_wait(c, 0) == QD_ERR_KILLED, (long) rc);
	d = spawn("g", 10, 27, QD_SPAWN_HELD, QD_SPAWN_CON_OWN);
	check("held child activated", d > 0 && qd_job_activate(d, 2) == 0, (long) d);
	rc = qd_wait(d, -1);
	check("activated held child ends with status 27", rc == 27, (long) rc);
	/* load failures leave nothing behind */
	rc = qd_spawn("NOSUCH.AOU", 0, 0, 0, QD_SPAWN_CON_OWN);
	check("spawn of a missing file fails (no such file)", rc < 0, (long) rc);
	rc = qd_spawn("BADMAG.AOU", 0, 0, 0, QD_SPAWN_CON_OWN);
	check("spawn of a malformed image fails (-21)", rc == -21, (long) rc);
	rc = qd_spawn("CHILDT.AOU", "a", 1, 0x80, QD_SPAWN_CON_OWN);
	check("spawn with an unknown flag refused (-34)", rc == -34, (long) rc);
	rc = qd_spawn("CHILDT.AOU", (const char *) 0x00100000u, 8, 0, QD_SPAWN_CON_OWN);
	check("spawn with arguments in kernel memory refused", rc < 0, (long) rc);
	/* the child limit (8 unreaped children) */
	nlim = 0;
	for (i = 0; i < 10; i++) {
		ids[i] = spawn("l", 4000, 30 + i, QD_SPAWN_HELD, QD_SPAWN_CON_OWN);
		if (ids[i] > 0)
			nlim++;
	}
	if (nlim == 8)
		check("child limit: 8 held children accepted, the 9th and 10th refused (limit)",
		      ids[8] == QD_PROC_ERR_LIMIT && ids[9] == QD_PROC_ERR_LIMIT, (long) ids[8]);
	else
		check("child limit: the job table filled first; the next spawns refused with 'no free job slot' (-29)",
		      nlim > 0 && ids[nlim] == -29 && ids[9] == -29, (long) nlim);
	for (i = 0; i < 8; i++)
		if (ids[i] > 0)
			qd_kill(ids[i]);
	rc = 0;
	for (i = 0; i < 8; i++)
		if (ids[i] > 0 && qd_wait(ids[i], 0) != QD_ERR_KILLED)
			rc++;
	check("all eight cancelled and reaped", rc == 0, (long) rc);
	/* three children ending at about the same time while the parent waits */
	a = spawn("p", 30, 41, 0, QD_SPAWN_CON_OWN);
	b = spawn("q", 30, 42, 0, QD_SPAWN_CON_OWN);
	c = spawn("r", 30, 43, 0, QD_SPAWN_CON_OWN);
	rc = qd_wait(a, -1) * 10000 + qd_wait(b, -1) * 100 + qd_wait(c, -1);
	check("three concurrent children: statuses 41, 42, 43", rc == 414243, (long) rc);
	qd_proc_info(0, &p1);
	check("no children left", p1.children == 0, (long) p1.children);
	check("frames back at the starting count", p1.pool_free == p0.pool_free, (long) p1.pool_free - (long) p0.pool_free);
	check("parent's own heap unchanged", p1.heap_used == p0.heap_used, (long) p1.heap_used);
}

int main(int argc, char **argv)
{
	int cycles = 2, k, rc;
	unsigned int heap0;
	struct qd_procinfo s0, s1;

	if (argc > 1 && strcmp(argv[1], "orphan") == 0) {
		rc = spawn("o", 300, 51, 0, QD_SPAWN_CON_OWN);
		printf("SPAWNT orphan child %d started; parent ends now\n", rc);
		return rc > 0 ? 0 : 3;
	}
	if (argc > 1 && strcmp(argv[1], "inter") == 0) {
		char buf[32];
		int n = sprintf(buf, "i") + 1;
		n += sprintf(buf + n, "0") + 1;
		n += sprintf(buf + n, "52") + 1;
		n += sprintf(buf + n, "read") + 1;
		rc = qd_spawn("CHILDT.AOU", buf, (unsigned int) n, 0, 3);
		printf("SPAWNT inter child %d on console 3, waiting\n", rc);
		rc = qd_wait(rc, -1);
		printf("SPAWNT inter child status %d\n", rc);
		return rc == 52 ? 0 : 3;
	}
	if (argc > 1)
		cycles = atoi(argv[1]);
	if ((qd_proc_caps() & QD_PROC_CAP_SPAWN) == 0) {
		printf("SPAWNT FAIL no process spawn in this kernel\n");
		return 3;
	}
	logch = qd_open_ex("WIN1:/SPAWNT.LOG", QD_O_WRITE | QD_O_CREATE | QD_O_TRUNC | QD_O_FILE);
	printf("SPAWNT start: %d cycles (log WIN1:/SPAWNT.LOG %s)\n", cycles, logch >= 0 ? "open" : "not open");       /* stdout's buffer is allocated before the counts are taken */
	heap0 = qd_free_bytes();
	qd_proc_info(0, &s0);
	for (k = 0; k < cycles; k++) {
		quiet = k > 0;
		matrix();
		printf("SPAWNT cycle %d done, checks %d fails %d\n", k + 1, checks, fails);
		{
			char l[80];
			sprintf(l, "SPAWNT cycle %d done, checks %d fails %d\n", k + 1, checks, fails);
			logline(l);
		}
	}
	qd_proc_info(0, &s1);
	check("after all cycles: kernel heap unchanged", qd_free_bytes() == heap0, (long) qd_free_bytes() - (long) heap0);
	check("after all cycles: frame pool unchanged", s1.pool_free == s0.pool_free, (long) s1.pool_free - (long) s0.pool_free);
	printf("SPAWNT %s checks=%d fails=%d\n", fails == 0 ? "PASS" : "FAIL", checks, fails);
	{
		char l[80];
		sprintf(l, "SPAWNT %s checks=%d fails=%d\n", fails == 0 ? "PASS" : "FAIL", checks, fails);
		logline(l);
		if (logch >= 0)
			qd_close(logch);
	}
	return fails == 0 ? 0 : 3;
}
