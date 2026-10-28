/* R11L guest host description for GCC 1.40 / cc1 / cccp running ON QDOS/386.
 * The legacy tree selects its (modernised) source paths with -DLEGACY_HOST64 and takes the host description
 * from host64.h; this file replaces the Linux LP64 one with QDOS/386's ILP32 libc (long is 32 bits).
 * Target description unchanged: tm.h -> config/tm-i386gas.h. */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#define FALSE 0
#define TRUE 1
#define HOST_BITS_PER_CHAR CHAR_BIT
#define HOST_BITS_PER_SHORT 16
#define HOST_BITS_PER_INT 32
#define HOST_BITS_PER_LONG 32
#define SUCCESS_EXIT_CODE 0
#define FATAL_EXIT_CODE 33
#define USG
#define BSTRING
#include "tm.h"
#define bcopy(a,b,c) memcpy(b,a,c)
#define bzero(a,b) memset(a,0,b)
#define bcmp(a,b,c) memcmp(a,b,c)
#ifndef alloca
#define alloca(n) __builtin_alloca(n)
#endif

extern char *xmalloc(size_t), *xrealloc(void *, size_t);

extern char *oballoc(), *permalloc(), *savealloc();

#include <stdarg.h>
void fatal(const char *, ...);

#define HOST_POINTER_UINT unsigned int
