/* Native Linux LP64 host; target configuration remains historical. */
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#define FALSE 0
#define TRUE 1
#define HOST_BITS_PER_CHAR CHAR_BIT
#define HOST_BITS_PER_SHORT 16
#define HOST_BITS_PER_INT 32
#define HOST_BITS_PER_LONG 64
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

#define HOST_POINTER_UINT uintptr_t

typedef char host_requires_lp64[(sizeof(void *)==8 && sizeof(long)==8 && sizeof(int)==4)?1:-1];

