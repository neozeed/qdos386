/* sys/types.h - port header (QDOS/386 has no POSIX types; only what the ported tools name). */
#ifndef QDOS386_PORT_SYS_TYPES_H
#define QDOS386_PORT_SYS_TYPES_H
#include <stddef.h>
typedef long off_t;
typedef unsigned long ino_t;
typedef long time_t_port;
#endif
