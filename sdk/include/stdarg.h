/* stdarg.h - QDOS/386 libc (R9).  The i386 cdecl layout: the arguments follow
 * the last named parameter on the stack, each rounded up to an int.  Written
 * in the style of the historical GCC 1.40 gstdarg.h for the i386 case, without
 * the compiler's builtins, so it needs no compiler support. */
#ifndef QDOS386_STDARG_H
#define QDOS386_STDARG_H
typedef char *va_list;
#define __qd_va_rounded(TYPE) \
  (((sizeof (TYPE) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))
#define va_start(AP, LASTARG) (AP = ((char *) &(LASTARG) + __qd_va_rounded (LASTARG)))
#define va_arg(AP, TYPE) (AP += __qd_va_rounded (TYPE), *((TYPE *) (AP - __qd_va_rounded (TYPE))))
#define va_end(AP) ((void) 0)
#endif
