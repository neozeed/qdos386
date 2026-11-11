/*
 * QDOS/386 R16A - decide where a FAT16 volume lives on a block device, from its sector 0.  Pure: it inspects only the
 * 512-byte buffer and a device sector count, so it is unit-testable on the host.  The block-volume adapter (win1.c)
 * reads sector 0, calls this, and applies the returned start offset; the FAT16 driver keeps seeing relative LBAs.
 */
#ifndef QDOS386_MBR_H
#define QDOS386_MBR_H

#define MBR_REFUSE      0        /* neither a FAT16 VBR nor a valid MBR with a usable FAT16 primary - do not mount */
#define MBR_SUPERFLOPPY 1        /* sector 0 is the FAT16 VBR: *lba = 0, *count = device */
#define MBR_PARTITION   2        /* one primary FAT16 partition: *lba = start, *count = size */

/* sector0: the 512 bytes at LBA 0.  dev_sectors: the physical device size.  On MBR_SUPERFLOPPY/MBR_PARTITION, *lba
 * and *count are set; *ncand (may be 0) receives the number of valid FAT16 primary candidates found (so the caller
 * can note when it chose the lowest-LBA one of several).  Returns one of the MBR_* codes. */
int mbr_select_fat16(const unsigned char *sector0, unsigned int dev_sectors,
                     unsigned int *lba, unsigned int *count, int *ncand);

#endif
