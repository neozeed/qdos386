/* unistd.h - port header: the low-level file calls the historical GCC tools use.  Implemented in
 * sdk/libc/posixio.c over QDOS/386 channels (no fork/exec, no pipes, no directories). */
#ifndef QDOS386_PORT_UNISTD_H
#define QDOS386_PORT_UNISTD_H
#include <stddef.h>
#include <sys/types.h>
int open(const char *name, int flags, ...);
int creat(const char *name, int mode);
int close(int fd);
int read(int fd, void *buf, unsigned int n);
int write(int fd, const void *buf, unsigned int n);
long lseek(int fd, long off, int whence);
int unlink(const char *name);
int isatty(int fd);
int getpid(void);
char *getcwd(char *buf, size_t size);
int chdir(const char *path);              /* R11M: the calling job's current directory */
int rmdir(const char *path);
#endif
