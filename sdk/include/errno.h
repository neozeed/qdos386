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

#endif
