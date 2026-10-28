/* sys/stat.h - port header: just enough stat for the ported tools (file type and size).
 * fstat/stat are implemented in sdk/libc/posixio.c; every file on WIN1_ is a regular file. */
#ifndef QDOS386_PORT_SYS_STAT_H
#define QDOS386_PORT_SYS_STAT_H
#include <sys/types.h>
struct stat {
	unsigned short st_mode;
	int st_uid;
	int st_gid;
	long st_size;
	long st_atime;
	long st_mtime;
	long st_ctime;
};
#define S_IFMT  0170000
#define S_IFREG 0100000
#define S_IFDIR 0040000
#define S_IFCHR 0020000
int fstat(int fd, struct stat *st);
int stat(const char *name, struct stat *st);
int mkdir(const char *path, int mode);     /* R11M; mode is ignored */
#endif
