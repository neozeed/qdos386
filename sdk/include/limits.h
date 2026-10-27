/* limits.h - QDOS/386 libc (R9).  i386: int and long are 32 bits. */
#ifndef QDOS386_LIMITS_H
#define QDOS386_LIMITS_H
#define CHAR_BIT 8
#define CHAR_MIN (-128)
#define CHAR_MAX 127
#define UCHAR_MAX 255
#define SHRT_MIN (-32768)
#define SHRT_MAX 32767
#define USHRT_MAX 65535
#define INT_MIN (-2147483647 - 1)
#define INT_MAX 2147483647
#define UINT_MAX 4294967295u
#define LONG_MIN (-2147483647L - 1L)
#define LONG_MAX 2147483647L
#define ULONG_MAX 4294967295UL
#endif
