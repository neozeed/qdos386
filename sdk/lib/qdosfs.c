/*
 * qdosfs.c - R11L: SDK wrappers for the writable WIN1_ volume.  A separate libqdos.a member: a
 * program that does not call these is byte-identical to one linked with the R11K library.
 * Freestanding C89.
 */
#include "qdos386.h"

#ifdef QD_MEMSTAT
extern void qd_ms_chan(int);
#endif
int qd_open_ex(const char *name, unsigned int flags)
{
	int r = (int) qtrap(QD_TRAP_CH_OPENX, (unsigned int) name, flags, 0);
#ifdef QD_MEMSTAT
	if (r > 0)
		qd_ms_chan(1);
#endif
	return r;
}

#define CTL_FLUSH 1u
#define CTL_TRUNC 2u
#define CTL_SIZE  3u
#define CTL_TELL  4u
#define FS_REMOVE 1u
#define FS_RENAME 2u
#define FS_FREE   3u
#define FS_CLUSTER 4u
#define FS_TOTAL  5u

int qd_flush(int ch)
{
	return (int) qtrap(QD_TRAP_CH_CTL, (unsigned int) ch, CTL_FLUSH, 0);
}

int qd_truncate(int ch, unsigned long size)
{
	return (int) qtrap(QD_TRAP_CH_CTL, (unsigned int) ch, CTL_TRUNC, (unsigned int) size);
}

long qd_fsize(int ch)
{
	return (long) (int) qtrap(QD_TRAP_CH_CTL, (unsigned int) ch, CTL_SIZE, 0);
}

long qd_tell(int ch)
{
	return (long) (int) qtrap(QD_TRAP_CH_CTL, (unsigned int) ch, CTL_TELL, 0);
}

int qd_remove(const char *name)
{
	return (int) qtrap(QD_TRAP_FS_OP, FS_REMOVE, (unsigned int) name, 0);
}

int qd_rename(const char *from, const char *to)
{
	return (int) qtrap(QD_TRAP_FS_OP, FS_RENAME, (unsigned int) from, (unsigned int) to);
}

long qd_fs_free_clusters(void)
{
	return (long) (int) qtrap(QD_TRAP_FS_OP, FS_FREE, (unsigned int) "WIN1_", 0);
}

long qd_fs_cluster_bytes(void)
{
	return (long) (int) qtrap(QD_TRAP_FS_OP, FS_CLUSTER, (unsigned int) "WIN1_", 0);
}

long qd_fs_total_clusters(void)
{
	return (long) (int) qtrap(QD_TRAP_FS_OP, FS_TOTAL, (unsigned int) "WIN1_", 0);
}

int qd_fs_writable(void)
{
	return qd_fs_cluster_bytes() > 0;
}

int qd_write_all(int ch, const void *buf, unsigned int n)
{
	const unsigned char *p = (const unsigned char *) buf;
	unsigned int done = 0;

	while (done < n) {
		unsigned int chunk = n - done;
		int r;
		if (chunk > QD_WRITE_MAX)
			chunk = QD_WRITE_MAX;
		r = qd_write(ch, p + done, chunk);
		if (r < 0)
			return done != 0 ? (int) done : r;
		if (r == 0)
			break;
		done += (unsigned int) r;
	}
	return (int) done;
}
