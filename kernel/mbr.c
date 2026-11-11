/* QDOS/386 R16A - the pure FAT16 volume locator (kernel/mbr.h).  No kernel dependencies. */
#include "mbr.h"

#define SECSZ 512u

static unsigned int rd16le(const unsigned char *p) { return (unsigned int) p[0] | ((unsigned int) p[1] << 8); }
static unsigned int rd32le(const unsigned char *p)
{
	return (unsigned int) p[0] | ((unsigned int) p[1] << 8) | ((unsigned int) p[2] << 16) | ((unsigned int) p[3] << 24);
}

/* Does this sector look like a FAT16 VBR (a valid BPB)?  Mirrors fat16_mount's first checks - enough to tell a
 * superfloppy VBR from an MBR, whose offset-11 bytes are boot code, not a 512-byte-per-sector BPB. */
static int looks_like_vbr(const unsigned char *s)
{
	unsigned int bps, spc, reserved, nfats;
	if (s[510] != 0x55 || s[511] != 0xaa)
		return 0;
	bps = rd16le(s + 11);
	spc = s[13];
	reserved = rd16le(s + 14);
	nfats = s[16];
	if (bps != SECSZ)
		return 0;
	if (spc == 0 || spc > 128 || (spc & (spc - 1)) != 0)
		return 0;
	if (reserved == 0 || nfats != 2)
		return 0;
	return 1;
}

int mbr_select_fat16(const unsigned char *s, unsigned int dev,
                     unsigned int *lba, unsigned int *count, int *ncand)
{
	int i, best = -1, n = 0;
	unsigned int best_start = 0, best_size = 0;

	*ncand = 0;
	if (dev == 0)
		return MBR_REFUSE;
	if (looks_like_vbr(s)) {                 /* unpartitioned superfloppy */
		*lba = 0;
		*count = dev;
		return MBR_SUPERFLOPPY;
	}
	if (s[510] != 0x55 || s[511] != 0xaa)    /* neither a VBR nor an MBR */
		return MBR_REFUSE;
	for (i = 0; i < 4; i++) {
		const unsigned char *e = s + 446 + i * 16;
		unsigned int type = e[4];
		unsigned int start = rd32le(e + 8);
		unsigned int size = rd32le(e + 12);
		if (type != 0x04 && type != 0x06 && type != 0x0e)
			continue;                        /* not a FAT16 partition type */
		if (start == 0 || size == 0)
			continue;                        /* zero-length, or would overlap the MBR */
		if (start >= dev || size > dev)
			continue;                        /* out of range */
		if (start + size < start)
			continue;                        /* 32-bit overflow */
		if (start + size > dev)
			continue;                        /* runs past the end of the device */
		n++;
		if (best < 0 || start < best_start) {
			best = i;
			best_start = start;
			best_size = size;
		}
	}
	*ncand = n;
	if (best < 0)
		return MBR_REFUSE;                   /* an MBR, but no usable FAT16 primary */
	*lba = best_start;
	*count = best_size;
	return MBR_PARTITION;
}
