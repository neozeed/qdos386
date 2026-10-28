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
#define QD_TRAP_CH_OPENX     0x2Au   /* R11L: open with flags (kernels built with -DQDOS_FS_WRITE) */
#define QD_TRAP_CH_CTL       0x2Bu   /* R11L: flush, truncate, size, tell */
#define QD_TRAP_FS_OP        0x2Cu   /* R11L: remove, rename, free space */
#define QD_TRAP_IMG_START    0x24u   /* R11H: start at a profile (0..3) or -1 for any */
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
int qd_start(const char *name, int profile);       /* R11H: tagged job id at a profile (0..3), -1 any; no arguments */
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

/* ---- R11K: paged launch (kernels built with -DQDOS_PAGING only) -------------------------------
 * qd_pg_start runs a standard slot-1 program (linked once at 0x240000, the SDK default) in a NEW
 * address space: its 1 MiB image window is private, everything else - kernel, heap, stacks, devices -
 * is shared and writable (this is not memory protection).  Arguments are one C string, as qd_run.
 * Returns a tagged job id (use the ordinary qd_job_* calls), or a negative error:
 *   -60 no free 1 MiB frame run   -61 image not linked at 0x240000   -62 refused to a paged caller
 *   -63 no free address-space record   -34 bad arguments   -20..-25 bad a.out   others as qd_run.
 * QD_PG_HELD starts the job inactive: it holds its resources but is not scheduled until
 * qd_job_activate(id, 2) - or is discarded with qd_pg_cancel(id).  A paged program cannot start
 * programs itself.  Not callable from a paged program.  On a kernel without paging every call
 * returns QD_PG_ERR_NOPAGING (qd_pg_available returns 0). */
#define QD_PG_HELD          1u
#define QD_PG_ERR_NOPAGING  (-70)
#define QD_PG_ERR_BADFLAGS  (-71)
#define QD_PG_ERR_NOTHELD   (-65)
#define QD_PG_CAP_START     0x0001
#define QD_PG_CAP_INFO      0x0002
#define QD_PG_CAP_ANYEXC    0x0004
#define QD_PG_CAP_HOLD      0x0008
#define QD_PG_CAP_TASKS     0x0010
#define QD_PG_CAP_HELD      0x0020
struct qd_pginfo {
	unsigned int paged;        /* 1 if the job has its own address space */
	unsigned int space;        /* opaque: equal exactly when two jobs share an address space */
	unsigned int vbase;        /* start of the private window (0x240000) */
	unsigned int vsize;        /* its size (1 MiB) */
	unsigned int dispatches;   /* times the job has been dispatched */
	unsigned int preempts;     /* timer preemptions of the job */
	unsigned int spaces_live;  /* live paged address spaces now */
	unsigned int held_bytes;   /* window bytes held by all paged processes */
};
int qd_pg_available(void);                                     /* 1 if this kernel can start paged programs */
int qd_pg_caps(void);                                          /* (ABI version << 24) | (max spaces << 16) | QD_PG_CAP_*, or QD_PG_ERR_NOPAGING */
int qd_pg_start(const char *name, const char *args, unsigned int flags);   /* job id, or a negative error */
int qd_pg_startv(const char *name, const char *packed, unsigned int len, unsigned int flags);
                                                               /* packed = len bytes of NUL-terminated strings: argv[1], argv[2], ... (the last NUL included) */
int qd_pg_cancel(int id);                                      /* 0: a held job that never ran is removed; QD_PG_ERR_NOTHELD otherwise */
int qd_pginfo(int id, struct qd_pginfo *out);                  /* id 0 = the caller; 0, or QD_ERR_NOJOB */


/* ---- R11L: writable WIN1_ volume (kernels built with -DQDOS_FS_WRITE only) ------------------------
 * A kernel without the option answers every call below with a negative error (the trap number is
 * unknown there); qd_fs_writable() tells the two apart.  Names are "WIN1_" + an uppercase 8.3 name.
 * Error codes (negative, passed through from the kernel):
 *   -80 write-protected device or read-only open   -81 disk full   -82 root directory full
 *   -83 already exists   -84 file is open elsewhere   -85 sector write failed
 * Rules: a file has many readers or one writer; remove/rename need it closed; seek never goes past
 * the end (use qd_truncate to extend with zeros); writes are write-through (each qd_write is on the
 * device when it returns, the directory entry at qd_flush/qd_close/job exit); at most 256 bytes
 * per qd_write call (qd_write_all chunks). */
#define QD_O_READ    0x01u
#define QD_O_WRITE   0x02u
#define QD_O_CREATE  0x04u
#define QD_O_TRUNC   0x08u
#define QD_O_APPEND  0x10u
#define QD_O_EXCL    0x20u
#define QD_ERR_WPROT   (-80)
#define QD_ERR_FULL    (-81)
#define QD_ERR_DIRFULL (-82)
#define QD_ERR_EXIST   (-83)
#define QD_ERR_BUSY    (-84)
#define QD_ERR_WIO     (-85)
#define QD_WRITE_MAX   256u
int qd_open_ex(const char *name, unsigned int flags);   /* channel id, or negative */
int qd_flush(int ch);                                   /* write the directory entry; 0 or negative */
int qd_truncate(int ch, unsigned long size);            /* shrink (frees clusters) or extend with zeros */
long qd_fsize(int ch);                                  /* size in bytes, or negative */
long qd_tell(int ch);                                   /* position, or negative */
int qd_remove(const char *name);                        /* "WIN1_NAME.EXT"; the file must be closed */
int qd_rename(const char *from, const char *to);
long qd_fs_free_clusters(void);                         /* free clusters, or negative */
long qd_fs_cluster_bytes(void);
long qd_fs_total_clusters(void);
int qd_fs_writable(void);                               /* 1 if this kernel has the write traps */
int qd_write_all(int ch, const void *buf, unsigned int n);   /* chunks of QD_WRITE_MAX; n, or negative */

#endif
