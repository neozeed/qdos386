/*
 * R11I: read-only host-file sector device.  See blockdev.h and docs/R11I-BLOCK-PROTOCOL.md.
 * Plain C89.  The image is opened with fopen(path, "rb"): O_RDONLY, and the device never writes.
 *
 * Limit: the host long type must hold the image size in bytes, so the image is at most
 * LONG_MAX bytes (2 GiB on a 32-bit host).  Larger images are refused by the seek check.
 */
#include <stdio.h>
#ifdef BLK_RW
#include <stdlib.h>
#endif
#include "blockdev.h"

static FILE *img = NULL;
static unsigned long nsec = 0;
static unsigned char lba_b[4];
static unsigned char buf[512];
static unsigned int dpos = 512;      /* 512 = no valid sector to read from */
static unsigned int status = BLK_NODISK;
#ifdef BLK_RW
static int writable = 0;
static unsigned char wbuf[512];
static unsigned int wpos = 0;
static unsigned int wover = 0;
static int fault_mode = 0;
static unsigned long fault_n = 0;
static unsigned long wdone = 0;
#endif

void blk_close(void)
{
	if (img != NULL)
		fclose(img);
	img = NULL;
	nsec = 0;
	dpos = 512;
	status = BLK_NODISK;
#ifdef BLK_RW
	writable = 0;
	wpos = 0;
	wover = 0;
	wdone = 0;
#endif
}

static int open_mode(const char *path, const char *mode)
{
	long size;
	unsigned int i;

	blk_close();
	if (path == NULL || path[0] == '\0')
		return BLK_OPEN_ARG;
	img = fopen(path, mode);
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

int blk_open(const char *path)
{
	return open_mode(path, "rb");
}

#ifdef BLK_RW
int blk_open_rw(const char *path)
{
	int r = open_mode(path, "r+b");
	if (r == BLK_OPEN_OK)
		writable = 1;
	return r;
}

void blk_set_fault(int mode, unsigned long n)
{
	fault_mode = mode;
	fault_n = n;
}

unsigned long blk_writes_done(void)
{
	return wdone;
}

/* Executes a write command for the LBA in lba_b. */
static void do_write(void)
{
	unsigned long lba;

	lba = (unsigned long) lba_b[0] | ((unsigned long) lba_b[1] << 8) |
	      ((unsigned long) lba_b[2] << 16) | ((unsigned long) lba_b[3] << 24);
	dpos = 512;
	if (img == NULL) {
		status = BLK_NODISK;
		return;
	}
	if (!writable) {
		status = BLK_WPROT;
		return;
	}
	if (lba >= nsec) {
		status = BLK_RANGE;
		return;
	}
	if (wpos != 512 || wover != 0) {
		status = BLK_WRSHORT;
		return;
	}
	if (fault_mode == 2 && wdone >= fault_n) {
		fflush(stdout);
		fprintf(stderr, "blockdev: injected power cut before write #%lu (lba %lu)\n", wdone + 1, lba);
		exit(77);
	}
	if (fault_mode == 1 && wdone >= fault_n) {
		status = BLK_WRIOERR;
		return;
	}
	if (fseek(img, (long) (lba * 512UL), SEEK_SET) != 0 || fwrite(wbuf, 1, 512, img) != 512) {
		clearerr(img);
		status = BLK_WRIOERR;
		return;
	}
	wdone++;
	status = BLK_OK;
}
#endif

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
#ifdef BLK_RW
		} else if (v == BLK_CMD_WRITE) {
			do_write();
		} else if (v == BLK_CMD_WRBEGIN) {
			wpos = 0;
			wover = 0;
			status = BLK_OK;
#endif
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
#ifdef BLK_RW
	if (addr == BLK_PORT_DATA) {
		if (wpos < 512)
			wbuf[wpos++] = v;
		else
			wover++;
		return;
	}
#endif
	/* STATUS, DATA and COUNT are read-only: the write is ignored (DATA: unless BLK_RW). */
}
