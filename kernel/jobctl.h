/*
 * QDOS/386 R11M - native job control (opt-in: -DQDOS_JOBCTL).
 *
 * A bounded, versioned view of the job table for programs (PS) and a safe way to end a job that was launched through
 * QT_IMG_LOAD / QT_IMG_START / QT_PG_START (KILL).  No kernel structure is exposed: QT_JOB_QUERY copies a fixed-format
 * record per job into a caller buffer.  See docs/R11M-JOB-CONTROL-ABI.md.  The matching .c file is empty unless QDOS_JOBCTL.
 */
#ifndef QDOS386_JOBCTL_H
#define QDOS386_JOBCTL_H

#define QT_JOB_QUERY   0x2D       /* a = buffer, b = buffer bytes, c = version (1) -> number of job records, or negative */
#define QT_JOB_KILL    0x2E       /* a = tagged job id -> 0, or negative (JC_ERR_*, -2 stale or unknown id) */

#define JC_VERSION     1u
#define JC_HDR_BYTES   16u        /* { version, entry_size, count, job_max } */
#define JC_ENTRY_BYTES 40u
#define JC_NAME_MAX    16

/* States (struct jc_entry.state). */
#define JC_S_RUN       1u         /* runnable (or running) */
#define JC_S_HELD      2u         /* created inactive and never dispatched */
#define JC_S_SLEEP     3u         /* suspended: timed or indefinite */
#define JC_S_DEAD      4u         /* ended, resources not yet reclaimed */
#define JC_S_CONWAIT   5u         /* sleeping in QT_CON_WAIT on its console */

/* Flags. */
#define JC_F_LAUNCHED  0x01u      /* created through the program launch traps (killable) */
#define JC_F_PAGED     0x02u      /* has a private image window (QT_PG_START) */
#define JC_F_SELF      0x04u      /* the calling job */

#define JC_NOCON       0xFFu      /* console field when the kernel has no virtual consoles */

struct jc_entry {                 /* 40 bytes, little endian, as copied to the caller */
	unsigned int id;              /* tagged job id */
	unsigned char slot;
	unsigned char state;
	unsigned char prio;           /* priority increment */
	unsigned char console;
	unsigned char flags;
	unsigned char pad[3];
	unsigned int dispatches;
	unsigned int age_ticks;       /* timer ticks since the job was created */
	unsigned int stack_bytes;
	char name[JC_NAME_MAX];       /* NUL-terminated, at most 15 characters */
};

#define JC_ERR_KILLED   (-66)     /* QT_JOB_EXIT of a job that was killed */
#define JC_ERR_PROTECT  (-67)     /* QT_JOB_KILL: not a launched job (SYSTEM, IDLE, the shell ...) */
#define JC_ERR_SELF     (-68)     /* QT_JOB_KILL: the caller itself */
#define JC_ERR_DEAD     (-69)     /* QT_JOB_KILL: the job has already ended */

#ifdef QDOS_JOBCTL
/* From jobs.c / the launch traps. */
void jobctl_slot_reset(int slot);                     /* job_alloc_sized: a new job in the slot */
void jobctl_note_launch(int id, unsigned int name_addr, unsigned int flags);   /* after a successful launch trap */
int jobctl_killed(unsigned int id);                   /* 1 if the job with this id was killed */
int jobctl_query(unsigned int buf, unsigned int bytes, unsigned int version);
int jobctl_kill(unsigned int id);
/* Provided by jobs.c. */
void job_mark_dead_other(int slot);                   /* a non-current job: inactive, dead, reclaimable at the next trap */
unsigned int job_dead_pending(void);
#endif

#endif
