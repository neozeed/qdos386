/*
 * QDOS/386 R4 - native channel manager.
 *
 * Three concepts are kept separate:
 *   - struct chan_object: the kernel's internal channel, heap-allocated and
 *     owned by the opening job's tagged id.  Job code never sees it.
 *   - the QDOS channel id: (tag << 16) | index.  index selects a slot in the
 *     kernel table; tag changes each time the slot is opened.  This is what a
 *     job holds.
 *   - the driver: the platform output backend, reached only through the
 *     struct chan_driver table.  The console driver calls the platform's
 *     con_putc (port 0x402 on the PC, MMIO on i386VM).
 *
 * The native service codes are in kernel/jobs.h (QT_CH_*, 0x14..0x17).
 * This is not the 68000 channel ABI; see docs/JS-ROM-CHANNEL-MAP.md.
 */
#ifndef QDOS386_CHANNEL_H
#define QDOS386_CHANNEL_H

#ifdef QDOS_FS_WRITE
#define CHAN_MAX 16                 /* R11L: channel table slots (cccp include nesting, cc1 + temp files) */
#else
#define CHAN_MAX 8                  /* channel table slots */
#endif
#define CHAN_MAX_WRITE 256          /* bytes per write call */
#define CHAN_NAME_MAX 18            /* name, NUL included: "WIN1_" + full 8.3 (12) + NUL (R11E) */
#define CHAN_TAG_MAX 0x7fff         /* tags wrap to 1 before this value */

/* Native error codes, negative.  -3 (no memory) and -6 (no free entry, channel
 * not open) match the JS ROM; -7 matches its "name not known" code. */
#define CHAN_ERR_NOSLOT  (-6)
#define CHAN_ERR_NAME    (-7)        /* no driver with that name */
#define CHAN_ERR_BADID  (-12)        /* invalid, closed or stale id */
#define CHAN_ERR_OWNER  (-13)        /* channel belongs to another job */
#define CHAN_ERR_RANGE  (-14)        /* buffer outside the arena or too long */
#define CHAN_ERR_NOTSUP (-15)        /* operation not supported by this driver */
#define CHAN_ERR_NOVOL  (-18)        /* WIN1_ volume not mounted */
#define CHAN_ERR_AGAIN  (-19)        /* CON: no input yet; wait with QT_CON_WAIT, then retry */

#include "fat16.h"

struct chan_object;

/* A driver is found by exact name, or by prefix when prefix is set: then the
 * whole name is passed to open, and the driver reads the part after its own
 * prefix.  A NULL read, seek or write means the operation is not supported. */
struct chan_driver {
	const char *name;
	int prefix;
	int (*open)(struct chan_object *ch, const char *name);
	void (*close)(struct chan_object *ch);
	int (*write)(struct chan_object *ch, const unsigned char *buf, unsigned int n);
	int (*read)(struct chan_object *ch, unsigned char *buf, unsigned int n);
	int (*seek)(struct chan_object *ch, unsigned long pos);
#ifdef QDOS_FS_WRITE
	/* R11L: ctl handles CHAN_CTL_*; fsop handles CHAN_FS_* on names (prefix drivers only). */
	int (*ctl)(struct chan_object *ch, unsigned int op, unsigned long arg);
	int (*fsop)(unsigned int op, const char *a, const char *b);
#endif
};

struct chan_object {
	unsigned int owner;              /* tagged job id of the opening job */
	unsigned int index;              /* table slot */
	unsigned int tag;                /* tag of this open */
	const struct chan_driver *drv;
	unsigned long bytes_out;         /* bytes written through this channel */
	struct fat16_file ff;            /* file state, used by WIN1_ channels only */
#ifdef QDOS_FS_WRITE
	unsigned int mode;               /* R11L: CHAN_O_* flags the channel was opened with */
#endif
};

/* Service entry points.  id and owner are as described above; owner is the
 * calling job's tagged id, supplied by the trap layer. */
int chan_open(unsigned int owner, unsigned int name_addr);
int chan_close(unsigned int id, unsigned int owner);
int chan_write(unsigned int id, unsigned int owner, unsigned int buf_addr, unsigned int n);
int chan_putc(unsigned int id, unsigned int owner, unsigned int c);
int chan_read(unsigned int id, unsigned int owner, unsigned int buf_addr, unsigned int n);
int chan_seek(unsigned int id, unsigned int owner, unsigned int pos);

#ifdef QDOS_FS_WRITE
/* R11L, opt-in (-DQDOS_FS_WRITE).  Open flags (same values as FAT_O_*): */
#define CHAN_O_READ   0x01u
#define CHAN_O_WRITE  0x02u
#define CHAN_O_CREATE 0x04u
#define CHAN_O_TRUNC  0x08u
#define CHAN_O_APPEND 0x10u
#define CHAN_O_EXCL   0x20u
/* Channel controls (QT_CH_CTL) */
#define CHAN_CTL_FLUSH 1u       /* write the directory entry -> 0 */
#define CHAN_CTL_TRUNC 2u       /* arg = new size -> 0 */
#define CHAN_CTL_SIZE  3u       /* -> size in bytes (files under 2 GiB) */
#define CHAN_CTL_TELL  4u       /* -> current position */
/* File-system operations on names (QT_FS_OP) */
#define CHAN_FS_REMOVE 1u       /* a = name */
#define CHAN_FS_RENAME 2u       /* a = old name, b = new name */
#define CHAN_FS_FREE   3u       /* a = any WIN1_ name or "WIN1_" -> free clusters */
#define CHAN_FS_CLUSTER 4u      /* -> bytes per cluster */
#define CHAN_FS_TOTAL  5u       /* -> data clusters on the volume */
int chan_open_ex(unsigned int owner, unsigned int name_addr, unsigned int flags);
int chan_ctl(unsigned int id, unsigned int owner, unsigned int op, unsigned int arg);
int chan_fsop(unsigned int op, unsigned int a_addr, unsigned int b_addr);
#endif

/* Job exit: close every channel owned by 'owner'.  Called by reclaim_dead
 * before the job's heap blocks are reclaimed.  Returns channels closed. */
unsigned int chan_close_owner(unsigned int owner);

/* Diagnostics for the tests. */
unsigned long chan_objects_live(void);
extern volatile unsigned long chan_opened;
extern volatile unsigned long chan_closed;
extern volatile unsigned long chan_closed_on_exit;
extern volatile unsigned long chan_rejected;

/* Shared with the test jobs: a user buffer must lie in the flat arena or the
 * kernel image below it.  This is a range check, not a protection check. */
int chan_user_range_ok(unsigned int addr, unsigned int n);

#endif
