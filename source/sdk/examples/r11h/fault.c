/* R11H-FIX1 FAULT: guest test for atomic rollback of a failed launch.
 * Linked for slot 1 and started with RUN FAULT.AOU.  It keeps MULTIA live on profile 2, then asks
 * for MULTIB on profile 3.  A test-only kernel (built with -DQDOS_TEST_FAULT_LAUNCH=N) fails the
 * second read of the N-th image load with FAT_ERR_IO (-40) after the profile region is reserved.
 * The release kernel never fails this way, so this program is only meaningful on the fault build.
 * Checks: the failed launch returns -40; the set of occupied job slots is unchanged; MULTIA is
 * still running and exits 3; profile 2 is still owned (COLLIDE refused, -28); the same profile 3
 * image starts again (no leaked region) and exits 7. */
#include <stdio.h>
#include "qdos386.h"

#define ERR_SLOTBUSY (-28)   /* APP_ERR_SLOTBUSY */
#define ERR_IO       (-40)   /* FAT_ERR_IO: the injected read failure */
#define NJOBS        6       /* JOB_MAX */

static int checks = 0;
static int fails = 0;

static void check(const char *what, int got, int want)
{
	checks++;
	if (got == want) {
		printf("FAULT: ok %s -> %d\n", what, got);
	} else {
		fails++;
		printf("FAULT: FAIL %s got %d want %d\n", what, got, want);
	}
}

static int occupied(void)
{
	int i, n = 0;

	for (i = 0; i < NJOBS; i++)
		if (qd_job_slot((unsigned int) i) > 0)
			n++;
	return n;
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

int main(void)
{
	int a, b, c, before, after, st;

	printf("FAULT: start\n");

	a = qd_start("MULTIA.AOU", 2);
	checks++;
	if (a <= 0) {
		fails++;
		printf("FAULT: FAIL MULTIA did not start (%d)\n", a);
		printf("FAULT: checks %d, failures %d\nFAULT: FAIL\n", checks, fails);
		return 1;
	}
	printf("FAULT: MULTIA live id %d on profile 2\n", a);

	before = occupied();
	b = qd_start("MULTIB.AOU", 3);
	after = occupied();
	check("faulted launch of MULTIB on profile 3", b, ERR_IO);
	check("occupied job slots unchanged by the failed launch", after, before);

	st = qd_job_exit_status(a);
	check("MULTIA still running after the failed launch", st, QD_ERR_RUNNING);
	check("profile 2 still owned by MULTIA", qd_start("COLLIDE.AOU", 2), ERR_SLOTBUSY);

	c = qd_start("MULTIB.AOU", 3);
	checks++;
	if (c <= 0) {
		fails++;
		printf("FAULT: FAIL retry on profile 3 refused (%d): region leaked\n", c);
	} else {
		printf("FAULT: ok retry on profile 3 started, id %d\n", c);
		check("retry exits 7", wait_exit(c), 7);
	}
	check("MULTIA exits 3, unharmed", wait_exit(a), 3);

	printf("FAULT: checks %d, failures %d\n", checks, fails);
	printf("FAULT: %s\n", fails == 0 ? "PASS" : "FAIL");
	return fails == 0 ? 0 : 1;
}
