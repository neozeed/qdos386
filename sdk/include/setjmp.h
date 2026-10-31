/* setjmp.h - QDOS/386 libc (R11D).  Native i386 setjmp/longjmp: the buffer
 * holds ebx, esi, edi, ebp, the caller's esp after the return, and the return
 * address.  The x87 state is not saved; see docs/R11D-LIBC-PORT-MAP.md. */
#ifndef QDOS386_SETJMP_H
#define QDOS386_SETJMP_H

typedef unsigned long jmp_buf[6];

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);

#endif
