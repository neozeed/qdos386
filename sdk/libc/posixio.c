/* posixio.c - QDOS/386 libc (R11L): the small POSIX file-descriptor layer the ported historical GCC
 * 1.40 / GAS 1.38 / binutils 1.94 tools need (open, read, write, close, lseek, fstat, stat, unlink,
 * isatty, fileno, fdopen, fcntl, bcopy/bzero/bcmp and a few id stubs).
 *
 * Model: a descriptor is a slot in a small table.  Descriptors 0, 1 and 2 are the console.  Any
 * other descriptor holds one QDOS channel on the WIN1_ volume (the kernel has CHAN_MAX channels per
 * system; keep NFD below that).  There are no directories, no links, no permissions, no pipes and no
 * dup: every file is a regular file named by an 8.3 name in the root directory (qd_fsname).
 * errno is set from the kernel error code (qd_set_errno).  C89. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "qdos386.h"

#define NFD 12                  /* descriptors 0..11; 0-2 are the console */
#define FD_CON   1
#define FD_FILE  2

struct fdent {
	int kind;                   /* 0 free, FD_CON, FD_FILE */
	int ch;                     /* channel (FD_FILE) */
	int flags;                  /* O_* access and append bits */
	int owned;                  /* 1 if a FILE stream (fdopen/fileno) shares the channel */
};

static struct fdent fdt[NFD];
static int fd_ready = 0;

int qd_fsname(const char *in, char *out, size_t outsz);
void qd_flush_std(void);
int qd_con_channel(void);       /* stdio.c */

static void fd_init(void)
{
	if (fd_ready)
		return;
	fd_ready = 1;
	fdt[0].kind = fdt[1].kind = fdt[2].kind = FD_CON;
	fdt[0].flags = O_RDONLY;
	fdt[1].flags = fdt[2].flags = O_WRONLY;
}

static struct fdent *fd_get(int fd)
{
	fd_init();
	if (fd < 0 || fd >= NFD || fdt[fd].kind == 0) {
		errno = EBADF;
		return NULL;
	}
	return &fdt[fd];
}

static int fd_alloc(void)
{
	int i;

	fd_init();
	for (i = 3; i < NFD; i++)
		if (fdt[i].kind == 0)
			return i;
	errno = EMFILE;
	return -1;
}

/* ---- hooks used by stdio.c ---------------------------------------------------------------------- */

/* fileno(): the descriptor for a stream's channel (0, 1, 2 for the console streams).  A stream
 * opened by fopen gets a descriptor on first request; it is released by qd_fd_forget at fclose. */
int qd_fd_for_channel(int ch)
{
	int i;

	fd_init();
	for (i = 3; i < NFD; i++)
		if (fdt[i].kind == FD_FILE && fdt[i].ch == ch)
			return i;
	i = fd_alloc();
	if (i < 0)
		return -1;
	fdt[i].kind = FD_FILE;
	fdt[i].ch = ch;
	fdt[i].flags = O_RDWR;
	fdt[i].owned = 1;
	return i;
}

/* fclose(): the channel is gone, drop any descriptor that named it. */
void qd_fd_forget(int ch)
{
	int i;

	if (!fd_ready)
		return;
	for (i = 3; i < NFD; i++)
		if (fdt[i].kind == FD_FILE && fdt[i].ch == ch) {
			fdt[i].kind = 0;
			fdt[i].ch = 0;
			fdt[i].owned = 0;
		}
}

/* fdopen(): the channel and access flags of a descriptor; marks it as shared with a stream. */
int qd_fd_take(int fd, int *flags)
{
	struct fdent *e = fd_get(fd);

	if (e == NULL)
		return -1;
	if (e->kind != FD_FILE) {
		errno = EINVAL;
		return -1;
	}
	e->owned = 1;
	if (flags != NULL)
		*flags = e->flags;
	return e->ch;
}

/* ---- open / close / read / write / lseek -------------------------------------------------------- */

int open(const char *name, int flags, ...)
{
	char full[FILENAME_MAX];
	unsigned int q = 0;
	int ch, fd, acc = flags & O_ACCMODE;

	if (name == NULL) {
		errno = EINVAL;
		return -1;
	}
	if (qd_fsname(name, full, sizeof full) != 0 || strcmp(full, "WIN1_") == 0) {
		errno = ENOENT;
		return -1;
	}
	fd = fd_alloc();
	if (fd < 0)
		return -1;
	if (acc == O_RDONLY && !(flags & (O_CREAT | O_TRUNC | O_APPEND))) {
		ch = qd_open(full);
	} else {
		if (acc == O_RDONLY || acc == O_RDWR)
			q |= QD_O_READ;
		if (acc == O_WRONLY || acc == O_RDWR)
			q |= QD_O_WRITE;
		if (flags & O_CREAT)
			q |= QD_O_CREATE;
		if (flags & O_TRUNC)
			q |= QD_O_TRUNC;
		if (flags & O_APPEND)
			q |= QD_O_APPEND;
		ch = qd_open_ex(full, q);
	}
	if (ch <= 0) {
		qd_set_errno(ch);
		return -1;
	}
	fdt[fd].kind = FD_FILE;
	fdt[fd].ch = ch;
	fdt[fd].flags = flags & (O_ACCMODE | O_APPEND);
	fdt[fd].owned = 0;
	return fd;
}

int creat(const char *name, int mode)
{
	(void) mode;
	return open(name, O_WRONLY | O_CREAT | O_TRUNC, 0);
}

int close(int fd)
{
	struct fdent *e = fd_get(fd);
	int r = 0;

	if (e == NULL)
		return -1;
	if (e->kind == FD_FILE) {
		r = qd_close(e->ch);
		e->kind = 0;
		e->ch = 0;
		e->owned = 0;
		if (r < 0) {
			qd_set_errno(r);
			return -1;
		}
		return 0;
	}
	/* The console descriptors stay (closing them is a no-op). */
	return 0;
}

int read(int fd, void *buf, unsigned int n)
{
	struct fdent *e = fd_get(fd);
	int r, ch;

	if (e == NULL)
		return -1;
	if ((e->flags & O_ACCMODE) == O_WRONLY) {
		errno = EBADF;
		return -1;
	}
	if (n == 0)
		return 0;
	ch = e->kind == FD_CON ? qd_con_channel() : e->ch;
	if (ch <= 0) {
		errno = EIO;
		return -1;
	}
	r = qd_read(ch, buf, n);
	if (r < 0) {
		qd_set_errno(r);
		return -1;
	}
	return r;
}

int write(int fd, const void *buf, unsigned int n)
{
	struct fdent *e = fd_get(fd);
	int r;

	if (e == NULL)
		return -1;
	if ((e->flags & O_ACCMODE) == O_RDONLY) {
		errno = EBADF;
		return -1;
	}
	if (n == 0)
		return 0;
	if (e->kind == FD_CON) {
		int ch = qd_con_channel();

		qd_flush_std();                 /* keep the order with buffered stdio output */
		if (ch <= 0) {
			errno = EIO;
			return -1;
		}
		r = qd_write(ch, buf, n);
		if (r < 0) {
			qd_set_errno(r);
			return -1;
		}
		return r;
	}
	r = qd_write_all(e->ch, buf, n);
	if (r < 0) {
		qd_set_errno(r);
		return -1;
	}
	return r;
}

long lseek(int fd, long off, int whence)
{
	struct fdent *e = fd_get(fd);
	long base, np;
	int r;

	if (e == NULL)
		return -1;
	if (e->kind != FD_FILE) {
		errno = ESPIPE;
		return -1;
	}
	if (whence == SEEK_SET) {
		base = 0;
	} else if (whence == SEEK_CUR) {
		base = qd_tell(e->ch);
	} else if (whence == SEEK_END) {
		base = qd_fsize(e->ch);
	} else {
		errno = EINVAL;
		return -1;
	}
	if (base < 0) {
		qd_set_errno((int) base);
		return -1;
	}
	np = base + off;
	if (np < 0) {
		errno = EINVAL;
		return -1;
	}
	r = qd_seek(e->ch, (unsigned long) np);
	if (r < 0) {
		qd_set_errno(r);
		return -1;
	}
	return np;
}

/* ---- file system ------------------------------------------------------------------------------- */

int unlink(const char *name)
{
	return remove(name) == 0 ? 0 : -1;
}

int fstat(int fd, struct stat *st)
{
	struct fdent *e = fd_get(fd);
	long sz;

	if (e == NULL)
		return -1;
	memset(st, 0, sizeof *st);
	if (e->kind == FD_CON) {
		st->st_mode = S_IFCHR | 0600;
		return 0;
	}
	sz = qd_fsize(e->ch);
	if (sz < 0) {
		qd_set_errno((int) sz);
		return -1;
	}
	st->st_mode = S_IFREG | 0644;
	st->st_size = sz;
	return 0;
}

int stat(const char *name, struct stat *st)
{
	char full[FILENAME_MAX];
	int ch;
	long sz;

	if (qd_fsname(name, full, sizeof full) != 0 || strcmp(full, "WIN1_") == 0) {
		errno = ENOENT;
		return -1;
	}
	ch = qd_open(full);
	if (ch <= 0) {
		qd_set_errno(ch);
		return -1;
	}
	sz = qd_fsize(ch);
	(void) qd_close(ch);
	if (sz < 0) {
		qd_set_errno((int) sz);
		return -1;
	}
	memset(st, 0, sizeof *st);
	st->st_mode = S_IFREG | 0644;
	st->st_size = sz;
	return 0;
}

int isatty(int fd)
{
	struct fdent *e = fd_get(fd);

	return e != NULL && e->kind == FD_CON;
}

int fcntl(int fd, int cmd, ...)
{
	struct fdent *e = fd_get(fd);

	if (e == NULL)
		return -1;
	if (cmd == F_GETFL)
		return e->flags;
	if (cmd == F_SETFL)
		return 0;
	errno = EINVAL;
	return -1;
}

int chmod(const char *name, int mode)
{
	struct stat st;

	(void) mode;
	return stat(name, &st);          /* no permissions: succeeds if the file exists */
}

int getpid(void) { return 1; }
int getuid(void) { return 0; }
int getgid(void) { return 0; }

/* ---- BSD memory functions ---------------------------------------------------------------------- */

void bcopy(const void *src, void *dst, unsigned int n)
{
	memmove(dst, src, n);
}

void bzero(void *p, unsigned int n)
{
	memset(p, 0, n);
}

int bcmp(const void *a, const void *b, unsigned int n)
{
	return memcmp(a, b, n);
}

/* There is one directory (the root of WIN1_) and no current-directory state. */
char *getcwd(char *buf, size_t size)
{
	if (buf == NULL || size < 2) {
		errno = ERANGE;
		return NULL;
	}
	buf[0] = '/';
	buf[1] = '\0';
	return buf;
}
