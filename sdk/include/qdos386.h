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
#define QD_TRAP_WALL         0x23u   /* R11F: wall clock, 0xFFFFFFFF if unavailable */
#define QD_TRAP_JOB_INFO     0x0Cu   /* R11E wrappers below use these existing traps */
#define QD_TRAP_JOB_ACTIVATE 0x0Au
#define QD_TRAP_JOB_CHPRIO   0x0Bu
#define QD_TRAP_JOB_REACT    0x09u
#define QD_TRAP_JOB_RECLAIMED 0x1Eu
#define QD_TRAP_IMG_LOAD     0x1Du
#define QD_TRAP_JOB_SELF     0x21u   /* R11E: added to the kernel in R11E */
#define QD_TRAP_JOB_SLOT     0x22u   /* R11E: added to the kernel in R11E */

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
unsigned long qd_wall_time(void);          /* R11F: epoch seconds, or 0xFFFFFFFF if no clock is configured */
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

/* ---- R11E additions -------------------------------------------------------
 * Error results are negative.  The job codes are the kernel's: -2 no such job
 * (stale or invalid tagged id), -16 job still running, -17 job ended without
 * QT_EXIT, -50 another job already waits for the console.  A job's exit status
 * is returned as is, so an exit status equal to -2, -16 or -17 cannot be told
 * apart from those errors (kernel ABI, documented). */
#define QD_ERR_NOJOB     (-2)
#define QD_ERR_RUNNING  (-16)
#define QD_ERR_NOSTATUS (-17)
#define QD_ERR_CONBUSY  (-50)
#define QD_EOF           (-1)  /* qd_getc: end of file (a byte is 0..255) */

/* Jobs.  Ids are tagged: the slot in the low 16 bits, a tag in the high bits.
 * A tag changes when a slot is reused, so a stale id fails with QD_ERR_NOJOB. */
int qd_self(void);                                  /* id of the calling job */
int qd_job_slot(unsigned int slot);                 /* id in that slot, or QD_ERR_NOJOB */
int qd_job_info(int id, unsigned int *inc, unsigned int *acc);   /* 0, or an error */
int qd_job_activate(int id, unsigned int inc);      /* 0; -1 if already active.  The increment must be non-zero (0 is inactive) */
int qd_job_chprio(int id, unsigned int inc);        /* 0.  inc 0 makes the job inactive: it is no longer scheduled, so do not use 0 on a running job */
int qd_job_reactivate(int id);                      /* 0; releases a suspended job */
int qd_job_suspend(long ticks);                     /* the calling job; < 0 = until released */
int qd_job_exit_status(int id);                     /* status, or a QD_ERR_* code */
int qd_job_reclaimed(int id);                       /* 1 if the job's slot is free */
int qd_run(const char *name, const char *args);     /* job id, or error; args may be NULL */
int qd_con_wait(unsigned char *flag);               /* sleep until console input (flag set) */

/* Heap, with the failure code kept (qd_alloc returns NULL only).  Blocks are
 * owned by the calling job and freed by the kernel when the job ends. */
int qd_alloc_ex(unsigned int bytes, void **out);    /* 0, or MEM_ERR_* */
int qd_mem_validate(void);                          /* 0 if the heap is consistent */

/* Channel helpers. */
int qd_getc(int ch);                                /* 0..255, QD_EOF at end, or an error */

/* WIN1_ root directory.  qd_dir_open opens "WIN1_" (a channel); qd_dir_next
 * reads one 16-byte record: a name of at most 12 characters (NUL-terminated
 * here) and the file size.  Returns 1 for a record, 0 at the end, or an error.
 * Only the root directory exists on the volume. */
struct qd_dirent {
	char name[13];
	unsigned long size;
};
int qd_dir_open(void);
int qd_dir_next(int ch, struct qd_dirent *e);

#endif
