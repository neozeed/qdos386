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

#endif
