#ifndef R11I_BLOCKDEV_H
#define R11I_BLOCKDEV_H

/*
 * R11I: read-only, host-file-backed 512-byte sector device for the i386VM harness.
 *
 * The device lives in the harness device page (0x800000..0x800FFF).  Every register is one byte
 * wide: the harness refuses 16- and 32-bit accesses there, so the guest must use byte accesses.
 * The protocol is synchronous (no busy state, no interrupts, no DMA).  Full description:
 * docs/R11I-BLOCK-PROTOCOL.md.
 *
 *   0x800010  CMD      W   0x01 = read the sector whose LBA is in 0x800011..0x800014
 *   0x800011  LBA0     W   LBA, little-endian (bytes 0..3 at 0x800011..0x800014); read back
 *   0x800012  LBA1     W
 *   0x800013  LBA2     W
 *   0x800014  LBA3     W
 *   0x800015  STATUS   R   status of the last command (BLK_*)
 *   0x800016  DATA     R   next byte of the 512-byte sector buffer; auto-increments; past the
 *                          end returns 0 and does not advance
 *   0x800017  COUNT0   R   sector count, little-endian (bytes 0..3 at 0x800017..0x80001A)
 *   0x800018  COUNT1   R
 *   0x800019  COUNT2   R
 *   0x80001A  COUNT3   R
 *
 * Writes to STATUS, DATA and COUNT are ignored.  A CMD write resets the DATA index.  On any
 * status other than BLK_OK the sector buffer is zero.
 *
 * Plain C89 with <stdio.h> only: the host unit tests build this file directly.
 */

#define BLK_PORT_FIRST   0x800010UL
#define BLK_PORT_CMD     0x800010UL
#define BLK_PORT_LBA0    0x800011UL
#define BLK_PORT_LBA3    0x800014UL
#define BLK_PORT_STATUS  0x800015UL
#define BLK_PORT_DATA    0x800016UL
#define BLK_PORT_COUNT0  0x800017UL
#define BLK_PORT_COUNT3  0x80001AUL
#define BLK_PORT_LAST    0x80001AUL

#define BLK_CMD_READ     0x01u
/* R11L (compiled in only with -DBLK_RW; a default build answers these as unknown commands):
 *   0x03 WRBEGIN  reset the write index of the sector buffer to 0
 *   DATA writes   store the next byte (auto-increment; bytes past 512 are dropped and counted)
 *   0x02 WRITE    write the 512-byte buffer to the LBA in 0x800011..0x800014; the buffer must be
 *                 complete (exactly 512 DATA writes since WRBEGIN) or the status is BLK_WRSHORT.
 * A device opened read-only answers WRITE with BLK_WPROT. */
#define BLK_CMD_WRITE    0x02u
#define BLK_CMD_WRBEGIN  0x03u
#define BLK_SECTOR_SIZE  512UL

/* STATUS values */
#define BLK_OK           0u   /* sector is in the buffer */
#define BLK_NODISK       1u   /* no image is open */
#define BLK_RANGE        2u   /* LBA >= sector count */
#define BLK_SHORT        3u   /* the host gave fewer than 512 bytes (end of file inside the sector) */
#define BLK_BADCMD       4u   /* unknown command */
#define BLK_IOERR        5u   /* host read error */
#define BLK_WPROT        6u   /* R11L: WRITE to a device that was opened read-only */
#define BLK_WRSHORT      7u   /* R11L: WRITE with an incomplete or overfull sector buffer */
#define BLK_WRIOERR      8u   /* R11L: host write error, or an injected fault */

/* blk_open results (0 is success; negative codes are host-side refusals) */
#define BLK_OPEN_OK        0
#define BLK_OPEN_ARG     (-1)  /* NULL or empty path */
#define BLK_OPEN_NOFILE  (-2)  /* cannot open for reading */
#define BLK_OPEN_EMPTY   (-3)  /* zero bytes */
#define BLK_OPEN_ALIGN   (-4)  /* size is not a multiple of 512 */
#define BLK_OPEN_SEEK    (-5)  /* cannot determine the size */

/* Opens path read-only ("rb") and sets the sector count.  Closes any previous image first. */
int blk_open(const char *path);

#ifdef BLK_RW
/* R11L: opens path read-write ("r+b") for the guest write protocol.  Same checks as blk_open. */
int blk_open_rw(const char *path);

/* R11L test faults.  mode 0 = none.  mode 1 ("io"): after n successful sector writes every
 * further WRITE fails with BLK_WRIOERR and the file is not changed.  mode 2 ("exit"): the
 * harness calls exit(77) when the (n+1)th WRITE arrives, before touching the file (a power
 * cut between two sector writes).  n counts WRITE commands that reached the file. */
void blk_set_fault(int mode, unsigned long n);

/* Number of sector writes that reached the file since the device was opened. */
unsigned long blk_writes_done(void);
#endif

/* Closes the image.  The device then reports BLK_NODISK and a zero count. */
void blk_close(void);

/* 1 if an image is open. */
int blk_present(void);

/* Sector count of the open image, 0 if none. */
unsigned long blk_sector_count(void);

/* 1 if addr lies in BLK_PORT_FIRST..BLK_PORT_LAST. */
int blk_in_range(unsigned long addr);

/* Byte read and write at a device address (addr must satisfy blk_in_range). */
unsigned char blk_read8(unsigned long addr);
void blk_write8(unsigned long addr, unsigned char v);

/* Status of the last command (BLK_OK after open). */
unsigned int blk_status(void);

#endif
