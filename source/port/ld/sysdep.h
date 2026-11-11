/* sysdep.h - R11L host description of BFD / the a.out linker for QDOS/386 (ILP32; the C89 SDK libc plus the port
 * headers).  Replaces bfd/hosts/h-host64.h, which assumes a POSIX LP64 host.  GCC 1.40 supports long long. */
#ifndef BFD_QDOS386_H
#define BFD_QDOS386_H
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>
#define DONTDECLARE_MALLOC
#define BYTES_IN_PRINTF_INT 4
typedef signed char int8e_type;
typedef unsigned char uint8e_type;
typedef short int16e_type;
typedef unsigned short uint16e_type;
typedef int int32e_type;
typedef unsigned int uint32e_type;
typedef signed char int8_type;
typedef unsigned char uint8_type;
typedef short int16_type;
typedef unsigned short uint16_type;
typedef int int32_type;
typedef unsigned int uint32_type;
typedef unsigned long long uint64e_type;
typedef unsigned long long uint64_type;
typedef long long int64_type;
#define uint64_typeLOW(x) ((uint32_type)(x))
#define uint64_typeHIGH(x) ((uint32_type)((x)>>32))
#include "fopen-same.h"
#endif
