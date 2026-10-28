/* QDOS/386 R5 application-side trap interface (see docs/R5-APP-ABI.md). */
#ifndef QDOS386_APPCALLS_H
#define QDOS386_APPCALLS_H

int qtrap(unsigned int fn, unsigned int a, unsigned int b, unsigned int c);

#define APP_QT_SUSPEND  0x08u
#define APP_QT_JOB_EXIT  0x19u
#define APP_QT_CH_OPEN   0x14u
#define APP_QT_CH_READ   0x1Au
#define APP_QT_CON_WAIT  0x1Cu
#define APP_QT_IMG_LOAD  0x1Du
#define APP_QT_JOB_RECLAIMED 0x1Eu
#define APP_QT_CH_CLOSE  0x15u
#define APP_QT_CH_WRITE  0x16u
#define APP_QT_EXIT      0x18u
#define APP_QT_APP_ARGS 0x1Fu     /* R8: argc; the word at a gets the argv array address */
#define APP_QT_TICKS    0x20u     /* R8: timer ticks since boot */
/* R11M (kernels built with -DQDOS_FS_WRITE): open with flags, controls, name operations */
#define APP_QT_CH_OPENX  0x2Au
#define APP_QT_CH_CTL    0x2Bu
#define APP_QT_FS_OP     0x2Cu
#define APP_QT_PG_START  0x25u     /* R11K paging: start a paged slot-1 program */
#define APP_QT_PG_CAPS   0x27u
#define APP_QT_JOB_QUERY 0x2Du     /* R11M job control */
#define APP_QT_JOB_KILL  0x2Eu
#define APP_QT_MEM_ALLOC      0x10u
#define APP_QT_MEM_FREE       0x11u
#define APP_QT_MEM_FREE_BYTES 0x12u
#define APP_QT_MEM_VALIDATE   0x13u

/* R8 argument limits (kernel/aout.h): argc includes argv[0] and is at most 16;
 * the packed argument strings are at most 432 bytes, name included. */
#define APP_ARG_MAX      16u
#define APP_ARG_STR_MAX  432u

#endif
