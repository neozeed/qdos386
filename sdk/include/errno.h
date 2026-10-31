/* errno.h - C89 error numbers (R11C).  Values follow the common historical
 * numbering for the codes the R11C library sets: EDOM and ERANGE.  errno is a
 * plain int in libc.a; it is set only by the functions documented in
 * docs/R11C-FORMAT-ABI.md and docs/R11C-LIBM-MAP.md, and is never set by the
 * kernel wrappers. */
#ifndef QD_ERRNO_H
#define QD_ERRNO_H

extern int errno;

#define EDOM   33
#define ERANGE 34
/* R11L: set by the file functions (fopen, fclose, fwrite, remove, rename, fseek ...) from the
 * kernel's error code; numbering follows the common historical values. */
#define ENOENT 2
#define ESRCH  3      /* R14A: named by Hack's (dead, !MULTIUSER_LOCK) veryold(); never set by the library */
#define EIO    5
#define EBADF  9
#define ENOMEM 12
#define EACCES 13
#define EBUSY  16
#define EEXIST 17
#define EXDEV  18
#define ENOTDIR 20
#define EISDIR 21
#define EINVAL 22
#define EMFILE 24
#define ENOSPC 28
#define ESPIPE 29
#define EROFS  30
#define ENAMETOOLONG 36
#define ENOSYS 38
#define ENOTEMPTY 39

/* Maps a kernel error code (negative, see qdos386.h) to an errno value and stores it in errno;
 * returns the errno value. */
int qd_set_errno(int kernel_error);

#endif
