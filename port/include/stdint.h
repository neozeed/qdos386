/* stdint.h - port header (not part of the C89 SDK): the exact-width names the legacy GCC tree uses, for ILP32. */
#ifndef QDOS386_PORT_STDINT_H
#define QDOS386_PORT_STDINT_H
typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned int uintptr_t;
typedef int intptr_t;
#define UINT32_MAX 0xffffffffU
#define INT32_MAX 2147483647
#define SIZE_MAX 0xffffffffU
#endif
