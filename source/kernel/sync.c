/* QDOS/386 R12C - storage SYNC.  See sync.h.  Empty unless QDOS_SYNC is defined.  Freestanding C89. */
#ifdef QDOS_SYNC
#include "sync.h"
#include "jobs.h"
#include "channel.h"
#include "win1.h"
#include "platform.h"

static unsigned int st[SYNC_STAT_WORDS];
static unsigned int interval_s;
static unsigned int interval_gen;

int sync_now(int periodic)
{
	unsigned int files = 0, err = 0;
	int rc, dev;

	st[1]++;
	if (periodic)
		st[8]++;
	if (win1_volume() == 0) {
		rc = SYNC_ERR_NOVOL;
		goto done;
	}
	rc = chan_flush_files(&files);       /* (1) directory entries of files open for writing */
	st[6] = files;
	st[11] += files;
	dev = disk_flush(&err);              /* (2) the drive's cache, even when (1) failed: keep what is there */
	if (dev == 0) {
		st[12]++;
	} else if (dev == -2) {
		st[13]++;
		st[7] = err;
		if (rc == 0)
			rc = SYNC_ERR_DEVICE;
	} else {
		st[14]++;
		if (rc == 0)
			rc = SYNC_ERR_TIMEOUT;
	}
done:
	st[4] = (unsigned int) rc;
	st[5] = (unsigned int) sched_ticks;
	if (rc == 0) {
		st[2]++;
	} else {
		st[3]++;
		if (periodic)
			st[9]++;
	}
	return rc;
}

unsigned int sync_interval(void)
{
	return interval_s;
}

unsigned int sync_interval_gen(void)
{
	return interval_gen;
}

int sync_trap(unsigned int op, unsigned int b, unsigned int c)
{
	unsigned int i, old;

	(void) c;
	switch (op) {
	case SYNC_OP_NOW:
		return sync_now(0);
	case SYNC_OP_STATUS:
		if (!chan_user_range_ok(b, SYNC_STAT_WORDS * 4u))
			return CHAN_ERR_RANGE;
		st[0] = 1;
		st[10] = interval_s;
		for (i = 0; i < SYNC_STAT_WORDS; i++)
			((unsigned int *) b)[i] = st[i];
		return 0;
	case SYNC_OP_INTERVAL:
		if (b > SYNC_INTERVAL_MAX)
			return -4;
		old = interval_s;
		interval_s = b;
		interval_gen++;
		return (int) old;
	default:
		return -15;
	}
}
#else
typedef int sync_off_unit;           /* an empty translation unit is not ISO C */
#endif /* QDOS_SYNC */
