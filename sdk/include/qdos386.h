/*
 * qdos386.h - QDOS/386 R8 application interface (SDK foundation).
 *
 * This is the stable interface to the existing OS services.  It is NOT a C
 * library: there is no stdio.h, no printf, no FILE, and qd_alloc / qd_free are
 * QDOS heap calls, not malloc.  Programs are freestanding C89, compiled with
 * the historical GCC 1.40 tree through qdos386-gcc (see sdk/README.md).
 *
 * Entry: crt0 calls  int main(int argc, char **argv)  and passes the return
 * value to QT_EXIT.  argv[0] is the program name; further entries are the
 * arguments given to RUN (quoted words stay together).
 */
#ifndef QDOS386_SDK_H
#define QDOS386_SDK_H

/* The trap entry (lib/qtrap.s): EAX = fn, EBX = a, ECX = b, EDX = c. */
int qtrap(unsigned int fn, unsigned int a, unsigned int b, unsigned int c);

/* Trap numbers (kernel/jobs.h). */
#define QD_TRAP_SUSPEND      0x08u
#define QD_TRAP_JOB_EXIT     0x19u
#define QD_TRAP_CH_OPEN      0x14u
#define QD_TRAP_CH_CLOSE     0x15u
#define QD_TRAP_CH_WRITE     0x16u
#define QD_TRAP_CH_PUTC      0x17u
#define QD_TRAP_EXIT         0x18u
#define QD_TRAP_CH_READ      0x1Au
#define QD_TRAP_CH_SEEK      0x1Bu
#define QD_TRAP_CON_WAIT     0x1Cu
#define QD_TRAP_MEM_ALLOC    0x10u
#define QD_TRAP_MEM_FREE     0x11u
#define QD_TRAP_MEM_FREE_BYTES 0x12u
#define QD_TRAP_MEM_VALIDATE 0x13u
#define QD_TRAP_APP_ARGS     0x1Fu
#define QD_TRAP_TICKS        0x20u

/* Limits of the argument vector (see docs/R8-APP-ABI.md). */
#define QD_ARGC_MAX          16u     /* argv[0] included */
#define QD_ARG_STR_MAX       432u    /* packed strings, name included */

/* Channel I/O reads at most this many bytes per call (QT_CH_READ). */
#define QD_READ_MAX          256u

/* Error codes returned by the calls below are the kernel's negative codes
 * (CHAN_ERR_*, FAT_ERR_*, MEM_ERR_*), passed through unchanged. */

/* Program control. */
void qd_exit(int status);                   /* does not return */
int qd_args(char ***argv);                  /* argc; *argv = the argument vector */
unsigned long qd_ticks(void);               /* timer ticks since boot (low 32 bits) */
void qd_yield(unsigned int ticks);          /* sleep for about this many ticks */

/* Heap (QDOS-specific; not malloc semantics: no realloc, no calloc, and the
 * block is not zeroed).  qd_alloc returns NULL on failure. */
void *qd_alloc(unsigned int bytes);
int qd_free(void *block);                   /* 0 on success, negative on error */
unsigned int qd_free_bytes(void);           /* free heap bytes */
int qd_heap_ok(void);                       /* 1 if the heap invariants hold */

/* Channels.  Names: "CON" (console) or "WIN1_" + an 8.3 file name on the
 * WIN1_ volume (read only).  qd_open returns a channel id (> 0) or negative. */
int qd_open(const char *name);
int qd_close(int ch);
int qd_write(int ch, const void *buf, unsigned int n);   /* bytes written */
int qd_putc(int ch, int c);
int qd_read(int ch, void *buf, unsigned int n);          /* bytes read, 0 at end */
int qd_seek(int ch, unsigned long pos);

/* Console convenience: write a NUL-terminated string to CON. */
int qd_puts(const char *s);

#endif
