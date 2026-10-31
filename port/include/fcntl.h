/* fcntl.h - port header: open() flags (the values are this port's own; see sdk/libc/posixio.c). */
#ifndef QDOS386_PORT_FCNTL_H
#define QDOS386_PORT_FCNTL_H
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_CREAT  0x100
#define O_TRUNC  0x200
#define O_APPEND 0x400
#define O_ACCMODE 3
#define F_GETFL 3
#define F_SETFL 4
int fcntl(int fd, int cmd, ...);
#define O_BINARY 0
#define O_TEXT   0
#endif
