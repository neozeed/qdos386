/*
 * QDOS/386 i386VM disk.
 *
 * Two backends, selected at build time:
 *
 *   default (R6-R11H)   a preloaded RAM disk.  The harness loads the whole image
 *                       file at guest address 0 and tools/make-vm-image.py puts
 *                       the FAT16 volume at RAMDISK_BASE.  Read only.
 *
 *   QDOS_I386VM_BLOCK   (R11I) a read-only host-file block device behind the
 *                       byte registers in blockreg.h.  The FAT16 volume is a
 *                       separate host file given to the harness; no volume is
 *                       in guest RAM.  Each sector transfer runs with interrupts
 *                       off: the device has one command, LBA and data pointer,
 *                       and a timer-driven job switch in the middle would let
 *                       another job's transfer interleave with it.
 *
 * Both present the same interface: disk_read_sector(lba, buf) returns 0 or a
 * negative error, and disk_sector_count() is the device size in 512-byte sectors.
 */
#include "platform.h"
#include "appmap.h"

#ifdef QDOS_I386VM_BLOCK

#include "blockreg.h"

extern unsigned int irq_save(void);
extern void irq_restore(unsigned int flags);

#define BLK(a) (*(volatile unsigned char *) (a))

int disk_read_sector(unsigned int lba, unsigned char *buf)
{
	unsigned int flags;
	unsigned int i;
	int rc = 0;

	flags = irq_save();
	BLK(BLK_REG_LBA0) = (unsigned char) lba;
	BLK(BLK_REG_LBA0 + 1) = (unsigned char) (lba >> 8);
	BLK(BLK_REG_LBA0 + 2) = (unsigned char) (lba >> 16);
	BLK(BLK_REG_LBA0 + 3) = (unsigned char) (lba >> 24);
	BLK(BLK_REG_CMD) = BLK_CMD_READ;
	if (BLK(BLK_REG_STATUS) != BLK_ST_OK) {
		rc = -1;
	} else {
		for (i = 0; i < 512; i++)
			buf[i] = BLK(BLK_REG_DATA);
	}
	irq_restore(flags);
	return rc;
}

unsigned long disk_sector_count(void)
{
	unsigned long n;

	n = (unsigned long) BLK(BLK_REG_COUNT0);
	n |= (unsigned long) BLK(BLK_REG_COUNT0 + 1) << 8;
	n |= (unsigned long) BLK(BLK_REG_COUNT0 + 2) << 16;
	n |= (unsigned long) BLK(BLK_REG_COUNT0 + 3) << 24;
	return n;
}

#else /* default: preloaded RAM disk (unchanged from R6) */

#define RAMDISK_SECTORS (RAMDISK_SIZE / 512u)

int disk_read_sector(unsigned int lba, unsigned char *buf)
{
	const unsigned char *src;
	unsigned int i;

	if (lba >= RAMDISK_SECTORS)
		return -1;
	src = (const unsigned char *) (RAMDISK_BASE + lba * 512u);
	for (i = 0; i < 512; i++)
		buf[i] = src[i];
	return 0;
}

unsigned long disk_sector_count(void)
{
	return RAMDISK_SECTORS;
}

#endif
