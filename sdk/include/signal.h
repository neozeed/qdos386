/* signal.h - QDOS/386 libc (R11D).  The signal names exist; no handler is
 * ever delivered, so signal() fails with SIG_ERR and raise() returns nonzero.
 * See docs/R11D-BLOCKED-APIS.md. */
#ifndef QDOS386_SIGNAL_H
#define QDOS386_SIGNAL_H

typedef int sig_atomic_t;

#define SIGHUP 1
#define SIGPIPE 13
#define SIGABRT 6
#define SIGFPE 8
#define SIGILL 4
#define SIGINT 2
#define SIGSEGV 11
#define SIGTERM 15

#define SIG_DFL ((void (*)(int)) 0)
#define SIG_IGN ((void (*)(int)) 1)
#define SIG_ERR ((void (*)(int)) -1)

void (*signal(int sig, void (*handler)(int)))(int);
int raise(int sig);

#endif
