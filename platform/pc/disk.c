/*
 * QDOS/386 R6 - Tiny386 PC disk: the FAT16 volume on the primary slave of the
 * PIIX3 IDE controller (ports 0x1F0-0x1F7, Tiny386 "hdb=" option).
 *
 * Only IDENTIFY DEVICE (0xEC) and READ SECTORS (0x20) are issued, both with
 * LBA28 addressing and polled PIO.  Nothing is written.  The data port is read
 * one 16-bit word at a time.
 */
#include "platform.h"

extern void outb(unsigned int port, unsigned int val);
extern unsigned int inb(unsigned int port);
extern unsigned int inw(unsigned int port);

#define IDE_DATA     0x1f0
#define IDE_COUNT    0x1f2
#define IDE_LBA0     0x1f3
#define IDE_LBA1     0x1f4
#define IDE_LBA2     0x1f5
#define IDE_DRIVE    0x1f6
#define IDE_STATUS   0x1f7        /* read: status */
#define IDE_COMMAND  0x1f7        /* write: command */

#define ST_ERR  0x01
#define ST_DRQ  0x08
#define ST_DF   0x20
#define ST_BSY  0x80

#define IDE_CMD_READ      0x20
#define IDE_CMD_WRITE     0x30        /* R11L, QDOS_FS_WRITE only */
#define IDE_CMD_IDENTIFY  0xec
#define IDE_SLAVE_LBA    0xf0    /* LBA mode, slave drive (bit 4) */
#define IDE_POLL_LIMIT   2000000u

static int ide_state = 0;        /* 0 unknown, 1 present, -1 absent */
static unsigned long ide_sectors = 0;

/* Wait until BSY clears.  Returns the status, or -1 on timeout. */
static int ide_wait_ready(void)
{
	unsigned int i;
	for (i = 0; i < IDE_POLL_LIMIT; i++) {
		unsigned int st = inb(IDE_STATUS);
		if (!(st & ST_BSY))
			return (int) st;
	}
	return -1;
}

/* Wait until DRQ is set (data ready).  Returns 0, or negative on error. */
static int ide_wait_drq(void)
{
	unsigned int i;
	for (i = 0; i < IDE_POLL_LIMIT; i++) {
		int st = ide_wait_ready();
		if (st < 0)
			return -1;
		if (st & (ST_ERR | ST_DF))
			return -2;
		if (st & ST_DRQ)
			return 0;
	}
	return -3;
}

static void ide_read_words(unsigned char *buf, int words)
{
	int i;
	for (i = 0; i < words; i++) {
		unsigned int w = inw(IDE_DATA);
		buf[2 * i] = (unsigned char) (w & 0xff);
		buf[2 * i + 1] = (unsigned char) (w >> 8);
	}
}

/* IDENTIFY DEVICE on the slave: records the LBA28 sector count. */
static void ide_identify(void)
{
	unsigned char id[512];
	int st;

	ide_state = -1;
	outb(IDE_DRIVE, IDE_SLAVE_LBA);
	st = ide_wait_ready();
	if (st <= 0 || st == 0xff)           /* no drive answers */
		return;
	outb(IDE_COMMAND, IDE_CMD_IDENTIFY);
	if (ide_wait_drq() != 0)
		return;
	ide_read_words(id, 256);
	ide_sectors = (unsigned long) id[120] | ((unsigned long) id[121] << 8) |
	              ((unsigned long) id[122] << 16) | ((unsigned long) id[123] << 24);
	ide_state = (ide_sectors != 0) ? 1 : -1;
}

unsigned long disk_sector_count(void)
{
	if (ide_state == 0)
		ide_identify();
	return ide_state == 1 ? ide_sectors : 0;
}

int disk_read_sector(unsigned int lba, unsigned char *buf)
{
	if (disk_sector_count() == 0 || lba >= ide_sectors)
		return -1;
	if (ide_wait_ready() < 0)
		return -2;
	outb(IDE_DRIVE, (unsigned int) (IDE_SLAVE_LBA | ((lba >> 24) & 0x0f)));
	outb(IDE_COUNT, 1);
	outb(IDE_LBA0, lba & 0xff);
	outb(IDE_LBA1, (lba >> 8) & 0xff);
	outb(IDE_LBA2, (lba >> 16) & 0xff);
	outb(IDE_COMMAND, IDE_CMD_READ);
	if (ide_wait_drq() != 0)
		return -3;
	ide_read_words(buf, 256);
	return 0;
}

#ifdef QDOS_FS_WRITE
/* R11L: WRITE SECTORS (0x30), LBA28, polled PIO, one sector.  After the data the controller goes busy
 * while it writes; a clear BSY with ERR/DF clear means the sector was accepted.  A write-protected
 * disk (Tiny386 opens hdb read-write; QEMU read-only drives) answers ERR: reported as -2. */
static void ide_write_words(const unsigned char *buf, int words)
{
	int i;
	for (i = 0; i < words; i++) {
		unsigned int w = (unsigned int) buf[2 * i] | ((unsigned int) buf[2 * i + 1] << 8);
		__asm__ volatile ("outw %%ax,%%dx" : : "a" (w), "d" (IDE_DATA));
	}
}

#ifdef QDOS_SYNC
#define IDE_ERROR         0x1f1
#define IDE_CMD_FLUSH     0xe7
#define IDE_FLUSH_LIMIT   50000000u      /* a host fsync can take long; still bounded */

int disk_flush(unsigned int *err)
{
	unsigned int i, st = ST_BSY;

	*err = 0;
	if (disk_sector_count() == 0)
		return -1;
	if (ide_wait_ready() < 0)
		return -1;
	outb(IDE_DRIVE, IDE_SLAVE_LBA);
	outb(IDE_COMMAND, IDE_CMD_FLUSH);
	for (i = 0; i < IDE_FLUSH_LIMIT; i++) {
		st = inb(IDE_STATUS);
		if (!(st & ST_BSY))
			break;
	}
	if (st & ST_BSY)
		return -1;
	if (st & (ST_ERR | ST_DF)) {
		*err = inb(IDE_ERROR) & 0xffu;
		return -2;
	}
	return 0;
}
#endif

int disk_write_sector(unsigned int lba, const unsigned char *buf)
{
	int st;

	if (disk_sector_count() == 0 || lba >= ide_sectors)
		return -1;
	if (ide_wait_ready() < 0)
		return -1;
	outb(IDE_DRIVE, (unsigned int) (IDE_SLAVE_LBA | ((lba >> 24) & 0x0f)));
	outb(IDE_COUNT, 1);
	outb(IDE_LBA0, lba & 0xff);
	outb(IDE_LBA1, (lba >> 8) & 0xff);
	outb(IDE_LBA2, (lba >> 16) & 0xff);
	outb(IDE_COMMAND, IDE_CMD_WRITE);
	st = ide_wait_ready();
	if (st < 0)
		return -1;
	if (st & (ST_ERR | ST_DF))
		return -2;
	if (!(st & ST_DRQ))
		return -1;
	ide_write_words(buf, 256);
	st = ide_wait_ready();
	if (st < 0)
		return -1;
	if (st & ST_DF)
		return -1;
	if (st & ST_ERR)
		return -2;
	return 0;
}
#endif
