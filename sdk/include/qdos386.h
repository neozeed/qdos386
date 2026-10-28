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
#define QD_O_FILE    0x80u     /* R11M: a regular file only; a directory is refused (QD_ERR_ISDIR) */
#define QD_O_DIRS    0x40u     /* R11M: a directory listing also returns subdirectories (size 0xFFFFFFFF) */
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

/* ---- R11M: directories and paths (docs/R11M-FAT-PATH-ABI.md; kernels built with -DQDOS_FS_WRITE) ----
 * Native path form: "WIN1:/SRC/HELLO.C" (absolute), "WIN1:SRC/HELLO.C" or "SRC/HELLO.C" (relative to the
 * job's current directory).  Case-insensitive; '/' separates; "." and ".." work; ".." at the root stays at
 * the root.  The legacy "WIN1_NAME" is always the flat uppercase root name.  Each job has a current
 * directory, inherited from its creator.  Error codes (negative): -87 not a directory, -88 is a directory,
 * -89 directory not empty, -90 path too long or too deep (64 characters, 16 components), -91 not allowed on
 * that target ('.', '..', the root, a directory that is some job's current directory returns -84),
 * -92 a directory cannot move to another parent; plus the R11L codes. */
#define QD_ERR_NOTDIR   (-87)
#define QD_ERR_ISDIR    (-88)
#define QD_ERR_NOTEMPTY (-89)
#define QD_ERR_DEPTH    (-90)
#define QD_ERR_INVAL    (-91)
#define QD_ERR_XDIR     (-92)
#define QD_PATH_MAX     64
struct qd_stat {
	unsigned long size;       /* bytes (0 for a directory) */
	unsigned int attr;        /* FAT attribute byte; 0x10 = directory */
	int is_dir;
};
int qd_mkdir(const char *path);
int qd_rmdir(const char *path);                         /* empty directory that is nobody's current directory */
int qd_chdir(const char *path);                         /* sets the calling job's current directory */
int qd_getcwd(char *buf, unsigned int size);            /* length of the absolute path ("/", "/SRC"), or negative */
int qd_stat(const char *path, struct qd_stat *st);      /* 0, or negative (-44 no such file) */
int qd_dir_open_path(const char *path, int with_dirs);  /* a listing channel for any directory; 0 flag: files only */

/* ---- R11M Stage 4: VFAT long file names (kernels built with -DQDOS_VFAT; docs/R11M-VFAT-LIMITATIONS.md) -----------
 * A name that is not a valid upper-case 8.3 name (more than 8.3 characters, spaces, mixed case that needs keeping,
 * other punctuation) is stored with a long-name chain and a generated 8.3 alias (BASE~N.EXT).  Both names open the
 * same file.  Supported long names: 1..63 printable ASCII characters, not \ / : * ? " < > |, no leading space or dot,
 * no trailing space or dot.  Lookup is case-insensitive.  Names that are already valid 8.3 names are stored upper case,
 * as before.  qd_dir_open_long lists a directory with both names: records of QD_DIRL_RECORD bytes. */
#define QD_DIRL_RECORD    96
#define QD_DIRL_HASLFN    1      /* flags: name is a long name */
#define QD_DIRL_ALIASONLY 2      /* flags: the entry has a long name that is not supported (non-ASCII, too long); name is the alias */
struct qd_dirent_long {
	char name[64];           /* the long name, or the 8.3 name when there is none */
	char alias[16];          /* the 8.3 alias ("SUPERB~1.C") */
	unsigned long size;      /* 0xFFFFFFFF for a subdirectory */
	unsigned int attr;
	unsigned int flags;
};
int qd_dir_open_long(const char *path);                 /* a long-record listing channel (files and directories); QD_ERR_NOTSUP without VFAT */
int qd_dir_next_long(int ch, struct qd_dirent_long *e); /* 1 for a record, 0 at the end, or an error */

/* ---- R11M: virtual text consoles (kernels built with -DQDOS_VCON; docs/R11M-CONSOLE-ABI.md) -------------------
 * Every job belongs to one of QD_CON_MAX 80x25 consoles; its CON channel reads and writes that console.  All
 * calls take a CON channel (qd_open("CON") or qd_con_channel()) and act on the console it is bound to.
 * ROWS AND COLUMNS ARE 0-BASED (row 0 is the top line, column 0 the left): qd_con_goto, qd_con_cursor,
 * qd_con_region, qd_con_snap.  qd_con_gotoxy1 is the 1-BASED ANSI-order (row, column) form for ported code.
 * Return values: 0 or a value >= 0 on success, negative on error (QD_ERR_NOTSUP -15 on a kernel without the
 * console option, QD_ERR_RANGE -14 for an argument out of range).  Arguments are checked here and by the kernel. */
#define QD_CON_MAX      4
#define QD_CON_ROWS     25
#define QD_CON_COLS     80
#define QD_CON_ECHO     0x01u    /* the kernel shows typed characters */
#define QD_CON_COOKED   0x02u    /* line discipline: BS and ^U edit, a read gets whole lines ending in '\n' */
#define QD_CLR_ALL      0        /* qd_con_clear: whole console, cursor home */
#define QD_CLR_EOL      1        /*   from the cursor to the end of the line */
#define QD_CLR_LINE     2        /*   the cursor's line */
#define QD_CLR_EOS      3        /*   from the cursor to the end of the console */
#define QD_ATTR(fg, bg) ((unsigned int) (fg) | ((unsigned int) (bg) << 4))   /* colours 0..15 / 0..7 (bit 7 of the byte blinks) */
#define QD_ERR_NOTSUP   (-15)
#define QD_ERR_RANGE    (-14)
int qd_con_goto(int ch, int row, int col);            /* 0-based; clamped to the console */
int qd_con_gotoxy1(int ch, int row, int col);         /* 1-based (row, column), as ANSI */
int qd_con_clear(int ch, int what);                   /* QD_CLR_* */
int qd_con_attr(int ch, int attr);                    /* sets the attribute for new text; returns the old one */
int qd_con_size(int ch, int *rows, int *cols);        /* 25 x 80 */
int qd_con_cursor(int ch, int *row, int *col);        /* the cursor position (0-based) */
int qd_con_show_cursor(int ch, int show);             /* 1 show, 0 hide */
int qd_con_mode(int ch, unsigned int flags);          /* QD_CON_*; returns the old flags (0 = raw, no echo: the default) */
int qd_con_region(int ch, int top, int bottom);       /* scroll region (inclusive rows); 0..24 restores the whole console */
int qd_con_number(int ch);                            /* the console number (0..3) this channel is bound to */
int qd_con_is_foreground(int ch);                     /* 1 if that console has the keyboard and the display */
int qd_con_focus(int ch, int con);                    /* make console con the foreground one */
int qd_con_child(int ch, int con);                    /* the next job this job creates goes to console con (-1 clears) */
int qd_con_snap(int ch, int con, int row, char *buf, int size);  /* text of one row of console con (-1 = own); length, buf NUL-terminated */
int qd_con_getc(int ch);                              /* blocking read of one byte (sleeps in the kernel console wait); QD_EOF at end */
int qd_con_diag(int ch, int mode);                    /* diagnostic mirror: 0 console 0 only, 1 none, 2 all (tagged); -1 reads */

/* ---- R11M: job control (kernels built with -DQDOS_JOBCTL; docs/R11M-JOB-CONTROL-ABI.md) -------------------------
 * qd_job_list copies one record per job into out[0..max-1] (at most 8 records) and returns the count.  A job is
 * LAUNCHED when it was created by qd_run / qd_start / qd_pg_start or by the shell's RUN / START; only launched jobs can be killed.
 * qd_job_kill ends another job: it becomes inactive at once; its address space, channels, heap blocks and stack are
 * released by the kernel's ordinary reclaim pass (use qd_job_reclaimed to see the slot free).  qd_job_exit_status of a
 * killed job is QD_ERR_KILLED.  Ownership is not checked: any job may kill any launched job except itself.
 * Errors: QD_ERR_NOJOB (-2) stale or unknown id; QD_ERR_PROTECT, QD_ERR_SELF, QD_ERR_ENDED. */
#define QD_JS_RUN       1u
#define QD_JS_HELD      2u
#define QD_JS_SLEEP     3u
#define QD_JS_DEAD      4u
#define QD_JS_CONWAIT   5u
#define QD_JF_LAUNCHED  0x01u
#define QD_JF_PAGED     0x02u
#define QD_JF_SELF      0x04u
#define QD_JC_NOCON     0xFFu
#define QD_ERR_KILLED   (-66)
#define QD_ERR_PROTECT  (-67)
#define QD_ERR_SELF     (-68)
#define QD_ERR_ENDED    (-69)
#define QD_ERR_NOJOBCTL (-72)    /* the kernel was built without job control */
struct qd_jobinfo {
	unsigned long id;            /* tagged job id */
	int slot;
	int state;                   /* QD_JS_* */
	int prio;
	int console;                 /* 0..3, QD_JC_NOCON without virtual consoles */
	int flags;                   /* QD_JF_* */
	unsigned long dispatches;
	unsigned long age_ticks;
	unsigned long stack_bytes;
	char name[16];
};
int qd_job_list(struct qd_jobinfo *out, int max);
int qd_job_find(unsigned long id, struct qd_jobinfo *out);   /* 1 found (out filled), 0 not in the table, or negative */
int qd_job_kill(unsigned long id);                           /* 0, or a negative code */

#endif
