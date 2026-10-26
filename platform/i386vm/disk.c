/*
 * QDOS/386 R6 - i386VM disk: a preloaded RAM disk.
 *
 * The harness loads the whole image file at guest address 0, and the image
 * builder (tools/make-vm-image.py) puts the FAT16 volume at RAMDISK_BASE.
 * The sector interface is the same as the Tiny386 IDE backend's: a 512-byte
 * read of sector lba into a buffer.  The harness itself is unchanged; there
 * is no storage device on it.  Read only.
 */
#include "platform.h"
#include "appmap.h"

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
