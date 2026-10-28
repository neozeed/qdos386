/* varargs.h - historical (pre-ANSI) variable arguments for the i386 cdecl stack layout, in the style of the
 * GCC 1.40 gvarargs.h without compiler builtins.  va_alist is the name of the last (int) parameter; the
 * arguments that follow it on the stack are read with va_arg.  Shares va_list/va_arg/va_end with <stdarg.h>. */
#ifndef QDOS386_PORT_VARARGS_H
#define QDOS386_PORT_VARARGS_H
#include <stdarg.h>
#undef va_start
#define va_alist __builtin_va_alist
#define va_dcl int __builtin_va_alist;
#define va_start(AP) (AP = (char *) &__builtin_va_alist)
#endif
