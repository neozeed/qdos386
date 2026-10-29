/*
 * QDOS/386 R12C - storage SYNC (opt-in: -DQDOS_SYNC, needs QDOS_FS_WRITE).  See docs/R12C-SYNC-DURABILITY.md.
 *
 * QT_SYNC op SYNC_OP_NOW writes, in this order: (1) the pending directory entry (size, first cluster) of every file
 * open for writing - all other FAT16 sectors (data, both FAT copies, directories) are already on the device when the
 * write call that changed them returned; (2) ATA FLUSH CACHE to the volume's drive, whose completion status is checked.
 * The whole operation runs inside one trap with interrupts off, so it is serialised with every other file system
 * operation and every write that completed before it is included.  It cannot reach the stdio buffers inside another
 * process (each process flushes its own: qd_sync() in libc does fflush(NULL) first).
 */
#ifndef QDOS386_SYNC_H
#define QDOS386_SYNC_H

#define QT_SYNC            0x39   /* a = SYNC_OP_*, b, c -> see below */
#define SYNC_OP_NOW        0u     /* -> 0, or a negative code below (the counters record it) */
#define SYNC_OP_STATUS     1u     /* b = buffer of SYNC_STAT_WORDS words -> 0 */
#define SYNC_OP_INTERVAL   2u     /* b = seconds (0 = off, 1..3600) -> the previous interval, or -4 */

#define SYNC_ERR_NOVOL     (-18)  /* no volume mounted */
#define SYNC_ERR_DEVICE    (-96)  /* the drive answered FLUSH CACHE with an error (or aborted it: not supported) */
#define SYNC_ERR_TIMEOUT   (-97)  /* the drive did not complete FLUSH CACHE in time */
/* a directory-entry write error is returned as the FAT code (-85 write error, -40 read error, ...) */

#define SYNC_STAT_WORDS    16u
/* status words: 0 version (1), 1 syncs, 2 ok, 3 failed, 4 last result, 5 last tick, 6 files written by the last sync,
 * 7 error register of the last device error, 8 periodic syncs, 9 periodic failures, 10 interval seconds,
 * 11 files written in total, 12 device flushes completed, 13 device errors, 14 device timeouts, 15 reserved */

#define SYNC_INTERVAL_MAX  3600u

int sync_now(int periodic);                        /* call with interrupts off */
int sync_trap(unsigned int op, unsigned int b, unsigned int c);
unsigned int sync_interval(void);                  /* seconds, 0 = periodic sync off */
unsigned int sync_interval_gen(void);              /* changes whenever the interval is set */

#endif
