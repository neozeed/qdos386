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

#define CHAN_MAX 8                  /* channel table slots */
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
};

struct chan_object {
	unsigned int owner;              /* tagged job id of the opening job */
	unsigned int index;              /* table slot */
	unsigned int tag;                /* tag of this open */
	const struct chan_driver *drv;
	unsigned long bytes_out;         /* bytes written through this channel */
	struct fat16_file ff;            /* file state, used by WIN1_ channels only */
};

/* Service entry points.  id and owner are as described above; owner is the
 * calling job's tagged id, supplied by the trap layer. */
int chan_open(unsigned int owner, unsigned int name_addr);
int chan_close(unsigned int id, unsigned int owner);
int chan_write(unsigned int id, unsigned int owner, unsigned int buf_addr, unsigned int n);
int chan_putc(unsigned int id, unsigned int owner, unsigned int c);
int chan_read(unsigned int id, unsigned int owner, unsigned int buf_addr, unsigned int n);
int chan_seek(unsigned int id, unsigned int owner, unsigned int pos);

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
