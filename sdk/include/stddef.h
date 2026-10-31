/* stddef.h - QDOS/386 libc (R9).  C89 types and macros for the i386 target. */
#ifndef QDOS386_STDDEF_H
#define QDOS386_STDDEF_H
typedef unsigned int size_t;
typedef int ptrdiff_t;
#ifndef NULL
#define NULL ((void *) 0)
#endif
#define offsetof(type, member) ((size_t) &((type *) 0)->member)
#endif
