/* ENVT.AOU (R12B Stage 1): the per-process environment from an ordinary C89 application.
 *   usage: ENVT [cycles]            the full matrix (default 2 cycles) with a leak check; needs PATH set by the caller
 *          ENVT show NAME ...       print NAME=value (or NAME absent) for each name; status = number absent
 *          ENVT mod PATHVALUE       (child) check PATH, change PATH and add ENVT_A_ONLY; status 10 if correct
 *          ENVT hold PATHVALUE T    (child) check PATH, sleep T ticks, check PATH again and that no later variable
 *                                   appeared; status 20 if correct
 *          ENVT quick PATHVALUE     (child) check PATH; status 30 if equal
 *          ENVT count N BYTES       (child) check that the environment has N variables and BYTES bytes; status 40
 * Every check prints "ENVT ok ..." / "ENVT FAIL ..." (also to WIN1:/ENVT.LOG); the end is "ENVT PASS checks=N". */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qdos386.h"

static int checks, fails, quiet, logch = -1;
static char path0[256];

static void logline(const char *s)
{
	if (logch >= 0)
		qd_write_all(logch, s, (unsigned int) strlen(s));
}

static void check(const char *what, int ok, long got)
{
	char line[200];

	checks++;
	sprintf(line, "ENVT %s %s (%ld)\n", ok ? "ok  " : "FAIL", what, got);
	logline(line);
	if (!ok) {
		fails++;
		printf("%s", line);
	} else if (!quiet) {
		printf("%s", line);
	}
}

static int is(const char *name, const char *want)
{
	const char *v = getenv(name);

	if (want == 0)
		return v == 0;
	return v != 0 && strcmp(v, want) == 0;
}

static int spawn2(const char *mode, const char *a, const char *b)
{
	char buf[600];
	int n;

	n = sprintf(buf, "%s", mode) + 1;
	n += sprintf(buf + n, "%s", a) + 1;
	if (b)
		n += sprintf(buf + n, "%s", b) + 1;
	return qd_spawn("ENVT.AOU", buf, (unsigned int) n, 0, QD_SPAWN_CON_OWN);
}

static void api(void)
{
	char buf[300], big[300], nm[40];
	unsigned int c0, b0, c, b;
	int i, rc, n;

	qd_env_summary(&c0, &b0);
	check("setenv lower-case name, getenv upper-case name", setenv("envt_a", "one", 1) == 0 && is("ENVT_A", "one"), 0);
	check("names compare without regard to case", is("Envt_A", "one") && is("envt_a", "one"), 0);
	rc = qd_env_list(0, buf, sizeof buf);
	n = -1;
	for (i = 0; qd_env_list((unsigned int) i, buf, sizeof buf) >= 0; i++)
		if (strcmp(buf, "ENVT_A=one") == 0)
			n = i;
	check("stored upper case: list shows ENVT_A=one", n >= 0, n);
	check("overwrite=0 keeps the old value", setenv("ENVT_A", "two", 0) == 0 && is("ENVT_A", "one"), 0);
	check("overwrite=1 replaces it", setenv("ENVT_A", "two", 1) == 0 && is("ENVT_A", "two"), 0);
	check("empty value is set (not absent)", setenv("ENVT_E", "", 1) == 0 && is("ENVT_E", ""), 0);
	check("qd_env_get of an empty value: length 0", qd_env_get("ENVT_E", buf, sizeof buf) == 0 && buf[0] == 0, 0);
	check("unsetenv removes it", unsetenv("ENVT_E") == 0 && is("ENVT_E", 0), 0);
	check("qd_env_unset of an absent name: -7", qd_env_unset("ENVT_E") == -7, (long) qd_env_unset("ENVT_E"));
	check("getenv of an absent name: NULL", getenv("ENVT_NEVER") == 0, 0);
	check("value with spaces, ; and = kept", setenv("ENVT_S", " a b;c=d ", 1) == 0 && is("ENVT_S", " a b;c=d "), 0);
	rc = qd_env_get("ENVT_S", buf, 4);
	check("truncated read: full length returned, copy NUL-terminated", rc == 9 && strcmp(buf, " a ") == 0, rc);
	check("bad name '' refused -15", qd_env_set("", "x") == -15, (long) qd_env_set("", "x"));
	check("bad name '1ABC' refused -15", qd_env_set("1ABC", "x") == -15, (long) qd_env_set("1ABC", "x"));
	check("bad name 'A-B' refused -15", qd_env_set("A-B", "x") == -15, (long) qd_env_set("A-B", "x"));
	check("bad name 'A=B' refused -15", qd_env_set("A=B", "x") == -15, (long) qd_env_set("A=B", "x"));
	memset(nm, 'N', 32);
	nm[32] = 0;
	check("32-character name refused -15", qd_env_set(nm, "x") == -15, (long) qd_env_set(nm, "x"));
	nm[31] = 0;
	check("31-character name accepted", qd_env_set(nm, "x") == 0 && qd_env_unset(nm) == 0, 0);
	memset(big, 'v', 256);
	big[256] = 0;
	check("256-byte value refused -4", qd_env_set("ENVT_V", big) == -4, (long) qd_env_set("ENVT_V", big));
	big[255] = 0;
	check("255-byte value accepted and read back", qd_env_set("ENVT_V", big) == 0 && is("ENVT_V", big), 0);
	check("NULL name refused -14", qd_env_set((const char *) 0, "x") == -14, (long) qd_env_set((const char *) 0, "x"));
	check("name in kernel memory refused -14", qd_env_set((const char *) 0x1000, "x") == -14,
	      (long) qd_env_set((const char *) 0x1000, "x"));
	check("value in kernel memory refused -14", qd_env_set("ENVT_K", (const char *) 0x1000) == -14,
	      (long) qd_env_set("ENVT_K", (const char *) 0x1000));
	check("get buffer in kernel memory refused -14", qd_env_get("ENVT_A", (char *) 0x1000, 16) == -14,
	      (long) qd_env_get("ENVT_A", (char *) 0x1000, 16));
	check("refused calls changed nothing", is("ENVT_K", 0) && is("ENVT_A", "two"), 0);
	/* list order */
	n = 1;
	for (i = 1; qd_env_list((unsigned int) i, buf, sizeof buf) >= 0; i++) {
		char prev[300];
		qd_env_list((unsigned int) i - 1, prev, sizeof prev);
		if (strcmp(prev, buf) >= 0)
			n = 0;
	}
	check("list is in name order", n, i);
	qd_env_summary(&c, &b);
	check("summary counts the list", c == (unsigned int) i, (long) c);
	qd_env_unset("ENVT_A");
	qd_env_unset("ENVT_S");
	qd_env_unset("ENVT_V");
	qd_env_summary(&c, &b);
	check("back to the starting environment", c == c0 && b == b0, (long) b - (long) b0);
	/* the variable-count limit */
	for (i = 0; i < 40; i++) {
		sprintf(nm, "ENVT_F%02d", i);
		rc = qd_env_set(nm, "f");
		if (rc != 0)
			break;
	}
	qd_env_summary(&c, &b);
	check("variable limit: 32 variables, the next refused -5", rc == -5 && c == 32, (long) c);
	check("full: a new name refused, the old ones intact", qd_env_set("ENVT_NEW", "x") == -5 && is("ENVT_F00", "f"), 0);
	check("full: replacing an existing value still works", qd_env_set("ENVT_F00", "g") == 0 && is("ENVT_F00", "g"), 0);
	for (i = 0; i < 40; i++) {
		sprintf(nm, "ENVT_F%02d", i);
		qd_env_unset(nm);
	}
	/* the byte limit */
	memset(big, 'b', 255);
	big[255] = 0;
	for (i = 0; i < 20; i++) {
		sprintf(nm, "ENVT_B%02d", i);
		rc = qd_env_set(nm, big);
		if (rc != 0)
			break;
	}
	qd_env_summary(&c, &b);
	check("byte limit: refused -5 before 2048 bytes are exceeded", rc == -5 && b <= 2048 && b + 266 > 2048, (long) b);
	for (i = 0; i < 20; i++) {
		sprintf(nm, "ENVT_B%02d", i);
		qd_env_unset(nm);
	}
	qd_env_summary(&c, &b);
	check("limits: back to the starting environment", c == c0 && b == b0, (long) c);
}

static void inherit(void)
{
	int a, b, rc;
	unsigned long heap0;
	struct qd_procinfo s0, s1;

	heap0 = qd_free_bytes();
	qd_proc_info(0, &s0);
	unsetenv("ENVT_LATE");
	unsetenv("ENVT_A_ONLY");
	b = spawn2("hold", path0, "150");
	a = spawn2("mod", path0, 0);
	check("two concurrent children started (same image)", a > 0 && b > 0, (long) a);
	rc = setenv("ENVT_LATE", "after", 1);
	check("parent changes its environment after the spawns", rc == 0, rc);
	rc = qd_wait(a, 2000);
	check("child A changed its own PATH correctly (status 10)", rc == 10, rc);
	rc = qd_poll(b);
	check("child B is still running after A ended (its next check comes after A's change)", rc == QD_PROC_ERR_TIMEOUT,
	      rc);
	check("parent: PATH unchanged by child A", is("PATH", path0), 0);
	check("parent: child A's new variable not visible", is("ENVT_A_ONLY", 0), 0);
	rc = qd_wait(b, 2000);
	check("child B: PATH unchanged by A, no later parent variable (status 20)", rc == 20, rc);
	unsetenv("ENVT_LATE");
	qd_proc_info(0, &s1);
	check("after two children: no child records left", s1.children == 0, (long) s1.children);
	check("after two children: kernel heap unchanged", qd_free_bytes() == heap0, (long) qd_free_bytes() - (long) heap0);
	check("after two children: frame pool unchanged", s1.pool_free == s0.pool_free,
	      (long) s1.pool_free - (long) s0.pool_free);
	rc = qd_wait(a, 0);
	check("stale child id refused", rc < 0 && rc != 10, rc);
}

static void repeated(void)
{
	int i, id, rc, ok = 1, ids[6];
	unsigned long heap0;
	struct qd_procinfo s0, s1;
	char nm[16], big[200];
	unsigned int c, bb;

	heap0 = qd_free_bytes();
	qd_proc_info(0, &s0);
	for (i = 0; i < 12; i++) {
		id = spawn2("quick", path0, 0);
		rc = id > 0 ? qd_wait(id, 2000) : id;
		if (rc != 30)
			ok = 0;
	}
	check("12 spawns in a row each saw the same PATH (status 30)", ok, 0);
	for (i = 0; i < 6; i++)
		ids[i] = spawn2("quick", path0, 0);
	ok = 1;
	for (i = 5; i >= 0; i--)
		if (ids[i] <= 0 || qd_wait(ids[i], 2000) != 30)
			ok = 0;
	check("6 concurrent children, waited in reverse order, all saw PATH", ok, 0);
	/* a large environment is copied whole */
	memset(big, 'z', 150);
	big[150] = 0;
	for (i = 0; i < 10; i++) {
		sprintf(nm, "ENVT_L%d", i);
		qd_env_set(nm, big);
	}
	qd_env_summary(&c, &bb);
	{
		char cs[16], bs[16], buf[40];
		int n;
		sprintf(cs, "%u", c);
		sprintf(bs, "%u", bb);
		n = sprintf(buf, "count") + 1;
		n += sprintf(buf + n, "%s", cs) + 1;
		n += sprintf(buf + n, "%s", bs) + 1;
		id = qd_spawn("ENVT.AOU", buf, (unsigned int) n, 0, QD_SPAWN_CON_OWN);
		rc = id > 0 ? qd_wait(id, 2000) : id;
	}
	check("a child gets the whole environment (count and bytes equal, status 40)", rc == 40, (long) bb);
	for (i = 0; i < 10; i++) {
		sprintf(nm, "ENVT_L%d", i);
		qd_env_unset(nm);
	}
	/* rollback: the environment copy fails (diagnostic injection) */
	qd_proc_info(0, &s1);
	{
		unsigned long h1 = qd_free_bytes();
		struct qd_procinfo s2;
		qtrap(0x37u, 0xFFFFFFFEu, 1u, 0u);
		id = spawn2("quick", path0, 0);
		qd_proc_info(0, &s2);
		check("spawn with a failed environment copy refused -30", id == -30, id);
		check("refused spawn: kernel heap, frames, child records unchanged",
		      qd_free_bytes() == h1 && s2.pool_free == s1.pool_free && s2.children == s1.children && s2.procs_live == s1.procs_live,
		      (long) qd_free_bytes() - (long) h1);
		id = spawn2("quick", path0, 0);
		rc = id > 0 ? qd_wait(id, 2000) : id;
		check("the next spawn works (status 30)", rc == 30, rc);
	}
	qd_proc_info(0, &s1);
	check("repeated spawns: kernel heap unchanged", qd_free_bytes() == heap0, (long) qd_free_bytes() - (long) heap0);
	check("repeated spawns: frame pool unchanged", s1.pool_free == s0.pool_free, (long) s1.pool_free - (long) s0.pool_free);
}

static int child(int argc, char **argv)
{
	const char *mode = argv[1];
	int bad = 0, t;

	if (strcmp(mode, "show") == 0) {
		int i, absent = 0;
		for (i = 2; i < argc; i++) {
			const char *v = getenv(argv[i]);
			if (v)
				printf("ENVT show %s=%s\n", argv[i], v);
			else {
				printf("ENVT show %s absent\n", argv[i]);
				absent++;
			}
		}
		return absent;
	}
	if (strcmp(mode, "count") == 0) {
		unsigned int c, b;
		qd_env_summary(&c, &b);
		return (argc > 3 && c == (unsigned int) atoi(argv[2]) && b == (unsigned int) atoi(argv[3]) && is("ENVT_L9", 0) == 0)
		       ? 40 : 41;
	}
	if (argc < 3)
		return 2;
	if (!is("PATH", argv[2]))
		bad++;
	if (strcmp(mode, "quick") == 0)
		return bad ? 31 : 30;
	if (strcmp(mode, "mod") == 0) {
		if (setenv("PATH", "/A/ONLY", 1) != 0 || setenv("ENVT_A_ONLY", "1", 1) != 0)
			bad++;
		if (!is("PATH", "/A/ONLY") || !is("ENVT_A_ONLY", "1"))
			bad++;
		printf("ENVT child mod: PATH now %s\n", getenv("PATH") ? getenv("PATH") : "(none)");
		return bad ? 11 : 10;
	}
	if (strcmp(mode, "hold") == 0) {
		t = argc > 3 ? atoi(argv[3]) : 100;
		qd_yield((unsigned int) t);
		if (!is("PATH", argv[2]) || !is("ENVT_A_ONLY", 0) || !is("ENVT_LATE", 0))
			bad++;
		printf("ENVT child hold: PATH still %s\n", getenv("PATH") ? getenv("PATH") : "(none)");
		return bad ? 21 : 20;
	}
	return 2;
}

int main(int argc, char **argv)
{
	int cycles = 2, k;
	unsigned long heap0;
	struct qd_procinfo s0, s1;
	const char *p;

	if (argc > 1 && (argv[1][0] < '0' || argv[1][0] > '9'))
		return child(argc, argv);
	if (argc > 1)
		cycles = atoi(argv[1]);
	p = getenv("PATH");
	if (p == 0) {
		printf("ENVT FAIL PATH is not set (the caller sets it before ENVT runs)\n");
		return 3;
	}
	strcpy(path0, p);
	logch = qd_open_ex("WIN1:/ENVT.LOG", QD_O_WRITE | QD_O_CREATE | QD_O_TRUNC | QD_O_FILE);
	printf("ENVT start: %d cycles, inherited PATH=%s\n", cycles, path0);
	heap0 = qd_free_bytes();
	qd_proc_info(0, &s0);
	for (k = 0; k < cycles; k++) {
		quiet = k > 0;
		api();
		inherit();
		repeated();
		printf("ENVT cycle %d done, checks %d fails %d\n", k + 1, checks, fails);
	}
	qd_proc_info(0, &s1);
	check("after all cycles: kernel heap unchanged", qd_free_bytes() == heap0, (long) qd_free_bytes() - (long) heap0);
	check("after all cycles: frame pool unchanged", s1.pool_free == s0.pool_free, (long) s1.pool_free - (long) s0.pool_free);
	check("after all cycles: PATH as inherited", is("PATH", path0), 0);
	{
		char l[80];
		sprintf(l, "ENVT %s checks=%d fails=%d\n", fails == 0 ? "PASS" : "FAIL", checks, fails);
		printf("%s", l);
		logline(l);
		if (logch >= 0)
			qd_close(logch);
	}
	return fails == 0 ? 0 : 3;
}
