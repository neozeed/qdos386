/* R11H MULTI: launcher, linked for slot 1 and started by the shell with RUN.
 * It starts MULTIA at profile 2 and MULTIB at profile 3 and checks that both are
 * resident together.  It tries COLLIDE on profile 2 while MULTIA is live (must be
 * refused: APP_ERR_ADDROCC), waits for both exits, then reuses the profiles and
 * checks that a stale job id is refused. */
#include <stdio.h>
#include "qdos386.h"

#define ERR_SLOTBUSY (-28)   /* profile owned by a live job (APP_ERR_SLOTBUSY) */

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

int main(void)
{
	int a, b, c, sa, sb, round, id;
	int fails = 0;
	int ids[8];
	int nids = 0, refused = 0;

	a = qd_start("MULTIA.AOU", 2);
	b = qd_start("MULTIB.AOU", 3);
	printf("MULTI: started A=%d B=%d\n", a, b);
	if (a <= 0 || b <= 0)
		fails++;

	c = qd_start("COLLIDE.AOU", 2);
	printf("MULTI: collide on profile 2 -> %d\n", c);
	if (c != ERR_SLOTBUSY)
		fails++;

	/* Refusals that must leave nothing behind.  HELLO is linked for slot 0 and
	 * MULTIA for profile 2: each is refused for the wrong origin (-35).  A missing
	 * file is refused too. */
	{
		int x, y, z;

		x = qd_start("HELLO.AOU", 2);
		y = qd_start("MULTIA.AOU", 3);
		z = qd_start("NOSUCH.AOU", 2);
		printf("MULTI: wrong origin HELLO@2 -> %d, MULTIA@3 -> %d, missing -> %d\n", x, y, z);
		if (x != -35 || y != -35 || z >= 0)
			fails++;
	}

	sa = wait_exit(a);
	sb = wait_exit(b);
	printf("MULTI: A exit %d B exit %d\n", sa, sb);
	if (sa != 3 || sb != 7)
		fails++;

	/* A job id is (tag << 16) | index.  The exit record of a finished job is
	 * kept under its full id, so the id is refused (QD_ERR_NOJOB) only once its
	 * table index has been given to a new job.  Each round starts a job on
	 * profile 3; the earlier ids are then checked against that rule, and at
	 * least one refusal must be seen, so the refusal path is exercised. */
	ids[nids++] = b;
	for (round = 0; round < 6; round++) {
		int k, st;

		id = qd_start("MULTIB.AOU", 3);
		if (id <= 0) {
			printf("MULTI: reuse round %d refused %d\n", round, id);
			fails++;
			break;
		}
		printf("MULTI: round %d job %d\n", round, id);
		if (wait_exit(id) != 7)
			fails++;
		ids[nids++] = id;
		for (k = 0; k < nids - 1; k++) {
			st = qd_job_exit_status(ids[k]);
			if (((unsigned int) ids[k] & 0xFFFFu) == ((unsigned int) id & 0xFFFFu)) {
				if (st != QD_ERR_NOJOB) {
					printf("MULTI: reused index, stale id %d not refused (%d)\n", ids[k], st);
					fails++;
				}
			} else if (st != 7) {
				printf("MULTI: live record of id %d lost (%d)\n", ids[k], st);
				fails++;
			}
		}
	}
	for (round = 0; round < nids - 1; round++)
		if (qd_job_exit_status(ids[round]) == QD_ERR_NOJOB)
			refused++;
	if (refused == 0) {
		printf("MULTI: no stale id was refused; refusal path not exercised\n");
		fails++;
	}
	printf("MULTI: stale ids refused %d\n", refused);
	printf("MULTI: %s\n", fails == 0 ? "PASS" : "FAIL");
	return fails == 0 ? 0 : 1;
}
