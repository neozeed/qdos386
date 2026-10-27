/* R11H NEG: guest negative tests.  Linked for slot 1 and started with RUN NEG.AOU.
 *
 * 1. Malformed images, each started at its profile.  The loader must refuse each one
 *    with the documented code, and must leave nothing behind: a good pair
 *    (MULTIA at profile 2, MULTIB at profile 3) must still run to its own exit codes
 *    afterwards.
 * 2. Profile contention.  While two images are live, every profile is refused with
 *    APP_ERR_SLOTBUSY (-28) and nothing is loaded.
 * 3. Repeated allocation and release: NCHURN cycles on profile 3.  Every id is checked
 *    against the reuse rule: an older id is refused (QD_ERR_NOJOB) only when a newer
 *    job has been given the same table index; otherwise its exit record must be kept.
 * 4. Invalid job ids are refused, not trusted.
 *
 * Not reachable from a guest and so not tested here (see docs/R11H-RUNTIME-REPORT.md):
 * APP_ERR_ADDROCC, APP_ERR_NOJOB and region exhaustion.  They are covered by the host
 * allocator tests where they can be reached.
 */
#include <stdio.h>
#include "qdos386.h"

#define NCHURN 40
#define MAXIDS (NCHURN + 2)
#define ERR_SLOTBUSY (-28)
#define ERR_NOPROFILE (-35)
#define ERR_NOFILE (-44)   /* FAT_ERR_NOFILE: no such root-directory entry */

static int checks = 0;
static int fails = 0;

static void check(const char *what, int got, int want)
{
	checks++;
	if (got != want) {
		fails++;
		printf("NEG: FAIL %s -> %d (want %d)\n", what, got, want);
	} else {
		printf("NEG: ok   %s -> %d\n", what, got);
	}
}

static int wait_exit(int id)
{
	int st;

	do {
		st = qd_job_exit_status(id);
		if (st == QD_ERR_RUNNING)
			qd_yield(2);
	} while (st == QD_ERR_RUNNING);
	return st;
}

/* Starts MULTIA at profile 2 and MULTIB at profile 3, and checks their exits. */
static void good_pair(const char *tag)
{
	int a, b, sa, sb;

	a = qd_start("MULTIA.AOU", 2);
	b = qd_start("MULTIB.AOU", 3);
	checks++;
	if (a <= 0 || b <= 0) {
		fails++;
		printf("NEG: FAIL good pair (%s) refused: %d %d\n", tag, a, b);
		return;
	}
	sa = wait_exit(a);
	sb = wait_exit(b);
	checks++;
	if (sa != 3 || sb != 7) {
		fails++;
		printf("NEG: FAIL good pair (%s) exits %d %d (want 3 7)\n", tag, sa, sb);
	} else {
		printf("NEG: ok   good pair (%s) exits 3 7\n", tag);
	}
}

struct bad {
	const char *name;
	int profile;
	int want;
	const char *why;
};

static const struct bad bads[] = {
	{ "BADMAG.AOU",  2, -21, "magic is not OMAGIC" },
	{ "RELOC.AOU",   2, -22, "relocation records present" },
	{ "BADENT.AOU",  2, -25, "entry is no profile base" },
	{ "ENTOFF.AOU",  2, -25, "entry inside a profile, not its base" },
	{ "BADSYM.AOU",  2, -23, "symbol table not a record multiple" },
	{ "BIGTXT.AOU",  2, -24, "text larger than the profile" },
	{ "TRUNC.AOU",   2, -20, "one byte short" },
	{ "TRAIL.AOU",   2, -20, "one trailing byte" },
	{ "SHORT.AOU",   2, -20, "shorter than the header" },
	{ "EMPTYX.AOU",  2, -20, "empty file" },
	{ "MULTIB.AOU",  2, ERR_NOPROFILE, "profile-3 image asked for profile 2" },
	{ "MULTIA.AOU",  4, ERR_NOPROFILE, "profile 4 does not exist" },
	{ "NOSUCH.AOU",  2, ERR_NOFILE, "no such file" },
};

static const int nbads = sizeof bads / sizeof bads[0];

int main(void)
{
	int i, a, b, id, n, r, k, j, st;
	int ids[MAXIDS];
	int refused = 0;

	printf("NEG: start\n");

	/* 1. Malformed images. */
	for (i = 0; i < nbads; i++) {
		id = qd_start(bads[i].name, bads[i].profile);
		check(bads[i].why, id, bads[i].want);
	}
	good_pair("after refusals");

	/* 2. Profile contention with two images live. */
	a = qd_start("MULTIA.AOU", 2);
	b = qd_start("MULTIB.AOU", 3);
	checks++;
	if (a <= 0 || b <= 0) {
		fails++;
		printf("NEG: FAIL contention setup %d %d\n", a, b);
	}
	check("COLLIDE on profile 2 while MULTIA live", qd_start("COLLIDE.AOU", 2), ERR_SLOTBUSY);
	check("MULTIB on profile 3 while MULTIB live", qd_start("MULTIB.AOU", 3), ERR_SLOTBUSY);
	/* HELLO is linked for slot 1, so asking for profile 0 is a wrong origin (-33, NOTSLOT0),
	 * refused before the profile is looked at.  Profile 0 contention itself needs a slot-0
	 * image, and the only one is the shell, so it is not tested here. */
	check("HELLO (slot 1 image) on profile 0", qd_start("HELLO.AOU", 0), -33);
	check("MULTI on profile 1 (this job owns it)", qd_start("MULTI.AOU", 1), ERR_SLOTBUSY);
	check("exit of MULTIA", wait_exit(a), 3);
	check("exit of MULTIB", wait_exit(b), 7);
	good_pair("after contention");

	/* 3. Repeated allocation and release on profile 3. */
	n = 0;
	for (r = 0; r < NCHURN; r++) {
		id = qd_start("MULTIB.AOU", 3);
		if (id <= 0) {
			fails++;
			printf("NEG: FAIL churn round %d refused %d\n", r, id);
			break;
		}
		ids[n++] = id;
		st = wait_exit(id);
		checks++;
		if (st != 7) {
			fails++;
			printf("NEG: FAIL churn round %d exit %d (want 7)\n", r, st);
		}
		/* Reuse rule, checked for every older id. */
		for (k = 0; k < n - 1; k++) {
			int reused = 0;

			for (j = k + 1; j < n; j++)
				if (((unsigned int) ids[k] & 0xFFFFu) == ((unsigned int) ids[j] & 0xFFFFu))
					reused = 1;
			st = qd_job_exit_status(ids[k]);
			checks++;
			if (reused) {
				if (st != QD_ERR_NOJOB) {
					fails++;
					printf("NEG: FAIL churn stale id %d not refused (%d)\n", ids[k], st);
				} else {
					refused++;
				}
			} else if (st != 7) {
				fails++;
				printf("NEG: FAIL churn live record %d lost (%d)\n", ids[k], st);
			}
		}
	}
	printf("NEG: churn %d cycles, stale ids refused %d\n", n, refused);
	checks++;
	if (n != NCHURN || refused == 0) {
		fails++;
		printf("NEG: FAIL churn incomplete or refusal path not exercised\n");
	}

	/* 4. Invalid ids. */
	check("exit status of id 0", qd_job_exit_status(0), QD_ERR_NOJOB);
	check("exit status of id -1", qd_job_exit_status(-1), QD_ERR_NOJOB);
	check("exit status of id 0x7FFF0001", qd_job_exit_status(0x7FFF0001), QD_ERR_NOJOB);

	printf("NEG: checks %d, failures %d\n", checks, fails);
	printf("NEG: %s\n", fails == 0 ? "PASS" : "FAIL");
	return fails == 0 ? 0 : 1;
}
