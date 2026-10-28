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
#define FS_MKDIR  6u
#define FS_RMDIR  7u
#define FS_CHDIR  8u
#define FS_GETCWD 9u
#define FS_STAT   10u

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

/* ---- R11M: directories ---------------------------------------------------------------------- */

int qd_mkdir(const char *path)
{
	return (int) qtrap(QD_TRAP_FS_OP, FS_MKDIR, (unsigned int) path, 0);
}

int qd_rmdir(const char *path)
{
	return (int) qtrap(QD_TRAP_FS_OP, FS_RMDIR, (unsigned int) path, 0);
}

int qd_chdir(const char *path)
{
	return (int) qtrap(QD_TRAP_FS_OP, FS_CHDIR, (unsigned int) path, 0);
}

int qd_getcwd(char *buf, unsigned int size)
{
	return (int) qtrap(QD_TRAP_FS_OP, FS_GETCWD, (unsigned int) buf, size);
}

int qd_stat(const char *path, struct qd_stat *st)
{
	unsigned char b[8];
	int r = (int) qtrap(QD_TRAP_FS_OP, FS_STAT, (unsigned int) path, (unsigned int) b);

	if (r < 0)
		return r;
	st->size = (unsigned long) b[0] | ((unsigned long) b[1] << 8) | ((unsigned long) b[2] << 16) |
	           ((unsigned long) b[3] << 24);
	st->attr = b[4];
	st->is_dir = r == 1;
	return 0;
}

#define CTL_LISTLONG 5u

int qd_dir_open_long(const char *path)
{
	int ch = qd_dir_open_path(path, 1);
	int r;

	if (ch <= 0)
		return ch;
	r = (int) qtrap(QD_TRAP_CH_CTL, (unsigned int) ch, CTL_LISTLONG, 0);
	if (r < 0) {
		qd_close(ch);
		return r;
	}
	return ch;
}

int qd_dir_next_long(int ch, struct qd_dirent_long *e)
{
	unsigned char rec[QD_DIRL_RECORD];
	int n = qd_read(ch, rec, QD_DIRL_RECORD);
	unsigned int i;

	if (n == 0)
		return 0;
	if (n < 0)
		return n;
	if (n != QD_DIRL_RECORD)
		return QD_ERR_RANGE;
	for (i = 0; i < 64; i++)
		e->name[i] = (char) rec[i];
	e->name[63] = '\0';
	for (i = 0; i < 16; i++)
		e->alias[i] = (char) rec[64 + i];
	e->alias[15] = '\0';
	e->size = (unsigned long) rec[80] | ((unsigned long) rec[81] << 8) | ((unsigned long) rec[82] << 16) |
	          ((unsigned long) rec[83] << 24);
	e->attr = (unsigned int) rec[84];
	e->flags = (unsigned int) rec[88];
	return 1;
}

int qd_dir_open_path(const char *path, int with_dirs)
{
	struct qd_stat st;
	int r = qd_stat(path, &st);

	if (r < 0)
		return r;
	if (!st.is_dir)
		return QD_ERR_NOTDIR;
	return qd_open_ex(path, with_dirs ? (QD_O_READ | QD_O_DIRS) : QD_O_READ);
}
