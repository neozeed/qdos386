/* errno.c - the C89 error indicator (R11C).  One int per program; the
 * functions that set it are listed in docs/R11C-LIBM-MAP.md. */
#include <errno.h>

int errno = 0;
