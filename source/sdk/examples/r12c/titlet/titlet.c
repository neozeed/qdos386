/* TITLET.AOU (R12C Stage 2): program titles from a C89 program that uses the C68 convention.
 *   usage: TITLET                    the title matrix (prints "TITLET ok/FAIL ...", ends with "TITLET PASS checks=N")
 *          TITLET hold T [title]     set the runtime title (if given), then run T ticks; every 100 ticks the title
 *                                    becomes "<title> tick k" (shows that a running program's title changes)
 *          TITLET child T title      (child of the matrix) set the title, sleep T ticks, end 7
 *          TITLET quick              (child) end 9 at once
 *          TITLET launch T PROG [args]  start PROG held (listed but not running) on this console, release it after T
 *                                    ticks, wait for it and print its status (keeps a short program such as Phoon
 *                                    visible to the task selector)
 * The program defines _prog_name, so its compiled title is "Hack-1.03"; _stack is defined as in C68 and is NOT used.
 * Every check also goes to WIN1:/TITLET.LOG. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qdos386.h"

char _prog_name[] = "Hack-1.03";
long _stack = 20L * 1024L;

static int checks, fails, logch = -1;
static unsigned char tq[16 + QD_TASK_ENTRY * 16];

static void check(const char *what, int ok, long got)
{
	char line[200];

	checks++;
	if (!ok)
		fails++;
	sprintf(line, "TITLET %s %s (%ld)\n", ok ? "ok  " : "FAIL", what, got);
	if (logch >= 0)
		qd_write_all(logch, line, (unsigned int) strlen(line));
	printf("%s", line);
}

/* The task-list record of job id, or 0. */
static struct qd_task *task_of(unsigned long id)
{
	int n = qd_task_list(tq, sizeof tq), i;

	for (i = 0; i < n; i++) {
		struct qd_task *t = (struct qd_task *) (tq + 16 + i * QD_TASK_ENTRY);
		if (t->id == id)
			return t;
	}
	return 0;
}

static int title_is(const char *want)
{
	char b[80];

	qd_get_title(b, sizeof b);
	return strcmp(b, want) == 0;
}

static int spawn(const char *a, const char *b, const char *c, unsigned int flags, const char *prog)
{
	char buf[200];
	int n = 0;

	if (a)
		n += sprintf(buf + n, "%s", a) + 1;
	if (b)
		n += sprintf(buf + n, "%s", b) + 1;
	if (c)
		n += sprintf(buf + n, "%s", c) + 1;
	return qd_spawn(prog, buf, (unsigned int) n, flags, QD_SPAWN_CON_OWN);
}

static void matrix(void)
{
	char b[80], longt[120];
	struct qd_task *t;
	int rc, a, c, i, ok;
	unsigned long self = (unsigned long) qd_self();

	check("compiled title from _prog_name: Hack-1.03", title_is("Hack-1.03"), 0);
	rc = qd_exe_name(b, sizeof b);
	check("executable name kept separately", rc > 0 && strstr(b, "TITLET") != 0, rc);
	t = task_of(self);
	check("task list shows Hack-1.03, source compiled, program TITLET.AOU",
	      t != 0 && strcmp(t->title, "Hack-1.03") == 0 && t->title_src == 1 && strstr(t->exe, "TITLET") != 0, t ? t->title_src : -1);
	rc = qd_set_title("Hack-1.03 level 2");
	check("runtime title set", rc == 17 && title_is("Hack-1.03 level 2"), rc);
	t = task_of(self);
	check("task list shows the runtime title, source runtime", t != 0 && strcmp(t->title, "Hack-1.03 level 2") == 0 && t->title_src == 2, 0);
	rc = qd_set_title("");
	check("empty title clears the runtime title: back to the compiled one", rc == 0 && title_is("Hack-1.03"), rc);
	qd_set_title("x");
	rc = qd_set_title(0);
	check("NULL clears it too", rc == 0 && title_is("Hack-1.03"), rc);
	memset(longt, 'L', 100);
	longt[100] = 0;
	rc = qd_set_title(longt);
	qd_get_title(b, sizeof b);
	check("100-byte title cut at 63", rc == 63 && strlen(b) == 63, rc);
	rc = qd_set_title("tab\there\001x\177y");
	check("control and non-ASCII bytes stored as '?'", rc == 12 && title_is("tab?here?x?y"), rc);
	qd_set_title("keep me");
	rc = qd_set_title((const char *) 0x1000);
	check("title in kernel memory refused -14, title unchanged", rc == -14 && title_is("keep me"), rc);
	rc = qd_get_title(b, 4);
	check("truncated read: full length, copy NUL-terminated", rc == 7 && strcmp(b, "kee") == 0, rc);
	rc = qd_get_title((char *) 0x1000, 16);
	check("get buffer in kernel memory refused -14", rc == -14, rc);
	qd_set_title(0);

	/* two copies with different runtime titles at once */
	a = spawn("child", "300", "Copy A", 0, "TITLET.AOU");
	c = spawn("child", "300", "Copy B", 0, "TITLET.AOU");
	qd_yield(40);
	ok = 0;
	{
		struct qd_task *ta = task_of((unsigned long) a), *tb = task_of((unsigned long) c);
		ok = ta != 0 && tb != 0 && strcmp(ta->title, "Copy A") == 0 && strcmp(tb->title, "Copy B") == 0 &&
		     strcmp(ta->exe, tb->exe) == 0 && ta->parent == self && tb->parent == self;
	}
	check("two copies of one program, two runtime titles, same program name, parent recorded", ok, a);
	check("copy A ends 7", qd_wait(a, 3000) == 7, 0);
	check("copy B ends 7", qd_wait(c, 3000) == 7, 0);

	/* rapid exit and slot reuse: a held child of a program without _prog_name never shows an old title */
	ok = 1;
	for (i = 0; i < 12; i++) {
		char tt[32];
		int k, h;
		struct qd_task *th;

		sprintf(tt, "Reuse %d", i);
		k = spawn("child", "0", tt, 0, "TITLET.AOU");
		if (k <= 0 || qd_wait(k, 3000) != 7)
			ok = 0;
		h = spawn("show", "PATH", 0, QD_SPAWN_HELD, "ENVT.AOU");
		th = task_of((unsigned long) h);
		if (h <= 0 || th == 0 || strcmp(th->title, "ENVT.AOU") != 0 || th->title_src != 0)
			ok = 0;
		if (h > 0)
			qd_kill(h);
		qd_wait(h, 3000);
	}
	check("12 exit/reuse rounds: a new job never shows the old title (held ENVT shows ENVT.AOU)", ok, i);
}

int main(int argc, char **argv)
{
	if (argc > 1 && strcmp(argv[1], "quick") == 0)
		return 9;
	if (argc > 3 && strcmp(argv[1], "child") == 0) {
		qd_set_title(argv[3]);
		qd_yield((unsigned int) atoi(argv[2]));
		return 7;
	}
	if (argc > 3 && strcmp(argv[1], "launch") == 0) {
		char buf[200];
		int n = 0, i, id, rc;

		for (i = 4; i < argc && n + (int) strlen(argv[i]) + 1 < (int) sizeof buf; i++)
			n += sprintf(buf + n, "%s", argv[i]) + 1;
		id = qd_spawn(argv[3], buf, (unsigned int) n, QD_SPAWN_HELD, QD_SPAWN_CON_OWN);
		printf("TITLET launch: %s held as job %d for %s ticks\n", argv[3], id, argv[2]);
		if (id <= 0)
			return 4;
		qd_yield((unsigned int) atoi(argv[2]));
		rc = qd_job_activate(id, 2);
		rc = rc == 0 ? qd_wait(id, -1) : rc;
		printf("TITLET launch: %s ended with status %d\n", argv[3], rc);
		return 0;
	}
	if (argc > 2 && strcmp(argv[1], "hold") == 0) {
		long t = atol(argv[2]), k;
		char base[64], b[80];

		strcpy(base, argc > 3 ? argv[3] : "Hack-1.03");
		if (argc > 3)
			qd_set_title(base);
		printf("TITLET hold %ld ticks as '%s'\n", t, base);
		for (k = 1; k * 100 <= t; k++) {
			qd_yield(100);
			sprintf(b, "%s tick %ld", base, k);
			qd_set_title(b);
		}
		return 0;
	}
	logch = qd_open_ex("WIN1:/TITLET.LOG", QD_O_WRITE | QD_O_CREATE | QD_O_TRUNC | QD_O_FILE);
	matrix();
	printf("TITLET %s checks=%d fails=%d\n", fails == 0 ? "PASS" : "FAIL", checks, fails);
	if (logch >= 0) {
		char l[64];
		sprintf(l, "TITLET %s checks=%d fails=%d\n", fails == 0 ? "PASS" : "FAIL", checks, fails);
		qd_write_all(logch, l, (unsigned int) strlen(l));
		qd_close(logch);
	}
	return fails == 0 ? 0 : 3;
}
