/*
 * R11I: read-only host-file sector device.  See blockdev.h and docs/R11I-BLOCK-PROTOCOL.md.
 * Plain C89.  The image is opened with fopen(path, "rb"): O_RDONLY, and the device never writes.
 *
 * Limit: the host long type must hold the image size in bytes, so the image is at most
 * LONG_MAX bytes (2 GiB on a 32-bit host).  Larger images are refused by the seek check.
 */
#include <stdio.h>
#include "blockdev.h"

static FILE *img = NULL;
static unsigned long nsec = 0;
static unsigned char lba_b[4];
static unsigned char buf[512];
static unsigned int dpos = 512;      /* 512 = no valid sector to read from */
static unsigned int status = BLK_NODISK;

void blk_close(void)
{
	if (img != NULL)
		fclose(img);
	img = NULL;
	nsec = 0;
	dpos = 512;
	status = BLK_NODISK;
}

int blk_open(const char *path)
{
	long size;
	unsigned int i;

	blk_close();
	if (path == NULL || path[0] == '\0')
		return BLK_OPEN_ARG;
	img = fopen(path, "rb");
	if (img == NULL)
		return BLK_OPEN_NOFILE;
	/* Unbuffered: every command reads the file as it is now.  A buffered stream would serve
	 * a sector from bytes read before the host file was truncated or replaced. */
	if (setvbuf(img, NULL, _IONBF, 0) != 0) {
		blk_close();
		return BLK_OPEN_SEEK;
	}
	if (fseek(img, 0L, SEEK_END) != 0) {
		blk_close();
		return BLK_OPEN_SEEK;
	}
	size = ftell(img);
	if (size < 0) {
		blk_close();
		return BLK_OPEN_SEEK;
	}
	if (size == 0) {
		blk_close();
		return BLK_OPEN_EMPTY;
	}
	if (size % 512L != 0L) {
		blk_close();
		return BLK_OPEN_ALIGN;
	}
	if (fseek(img, 0L, SEEK_SET) != 0) {
		blk_close();
		return BLK_OPEN_SEEK;
	}
	nsec = (unsigned long) (size / 512L);
	for (i = 0; i < 4; i++)
		lba_b[i] = 0;
	for (i = 0; i < 512; i++)
		buf[i] = 0;
	dpos = 512;
	status = BLK_OK;
	return BLK_OPEN_OK;
}

int blk_present(void)
{
	return img != NULL;
}

unsigned long blk_sector_count(void)
{
	return nsec;
}

unsigned int blk_status(void)
{
	return status;
}

int blk_in_range(unsigned long addr)
{
	return addr >= BLK_PORT_FIRST && addr <= BLK_PORT_LAST;
}

/* Executes a read command for the LBA in lba_b. */
static void do_read(void)
{
	unsigned long lba;
	size_t n;
	unsigned int i;

	lba = (unsigned long) lba_b[0] | ((unsigned long) lba_b[1] << 8) |
	      ((unsigned long) lba_b[2] << 16) | ((unsigned long) lba_b[3] << 24);
	dpos = 0;
	for (i = 0; i < 512; i++)
		buf[i] = 0;
	if (img == NULL) {
		status = BLK_NODISK;
		return;
	}
	if (lba >= nsec) {
		status = BLK_RANGE;
		return;
	}
	if (fseek(img, (long) (lba * 512UL), SEEK_SET) != 0) {
		status = BLK_IOERR;
		return;
	}
	n = fread(buf, 1, 512, img);
	if (n == 512) {
		status = BLK_OK;
		return;
	}
	for (i = 0; i < 512; i++)
		buf[i] = 0;
	if (ferror(img)) {
		clearerr(img);
		status = BLK_IOERR;
	} else {
		clearerr(img);
		status = BLK_SHORT;
	}
}

unsigned char blk_read8(unsigned long addr)
{
	unsigned long k;

	if (addr == BLK_PORT_CMD)
		return 0;
	if (addr >= BLK_PORT_LBA0 && addr <= BLK_PORT_LBA3)
		return lba_b[addr - BLK_PORT_LBA0];
	if (addr == BLK_PORT_STATUS)
		return (unsigned char) status;
	if (addr == BLK_PORT_DATA) {
		if (dpos < 512) {
			return buf[dpos++];
		}
		return 0;
	}
	if (addr >= BLK_PORT_COUNT0 && addr <= BLK_PORT_COUNT3) {
		k = (addr - BLK_PORT_COUNT0) * 8;
		return (unsigned char) ((nsec >> k) & 0xFFUL);
	}
	return 0;
}

void blk_write8(unsigned long addr, unsigned char v)
{
	unsigned int i;

	if (addr == BLK_PORT_CMD) {
		if (v == BLK_CMD_READ) {
			do_read();
		} else {
			for (i = 0; i < 512; i++)
				buf[i] = 0;
			dpos = 0;
			status = BLK_BADCMD;
		}
		return;
	}
	if (addr >= BLK_PORT_LBA0 && addr <= BLK_PORT_LBA3) {
		lba_b[addr - BLK_PORT_LBA0] = v;
		return;
	}
	/* STATUS, DATA and COUNT are read-only: the write is ignored. */
}
