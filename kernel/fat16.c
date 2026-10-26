/*
 * QDOS/386 R6 - shared FAT16 reader.  See fat16.h.
 *
 * Sector buffers are 512-byte locals; at most one is live per function, so the
 * stack use stays small and no state is shared between channels.
 */
#include "fat16.h"

#define FAT16_MIN_CLUSTERS 4085u     /* below this the volume is FAT12 */
#define FAT16_MAX_CLUSTERS 65524u    /* above this it is not FAT16 */
#define FAT_EOC_MIN        0xfff8u   /* end-of-chain markers */
#define FAT_BAD            0xfff7u

static unsigned int rd16(const unsigned char *p)
{
	return (unsigned int) p[0] | ((unsigned int) p[1] << 8);
}

static unsigned long rd32(const unsigned char *p)
{
	return (unsigned long) p[0] | ((unsigned long) p[1] << 8) |
	       ((unsigned long) p[2] << 16) | ((unsigned long) p[3] << 24);
}

static int same11(const unsigned char *a, const unsigned char *b)
{
	unsigned int i;
	for (i = 0; i < 11; i++)
		if (a[i] != b[i])
			return 0;
	return 1;
}

/* Every sector access goes through here: the sector must lie in the volume. */
static int rd_sector(struct fat16_vol *v, unsigned int lba, unsigned char *buf)
{
	if (lba >= v->total_sectors)
		return FAT_ERR_RANGE;
	if (v->read(v->ctx, lba, buf) != 0)
		return FAT_ERR_IO;
	return FAT_OK;
}

/* Next cluster of c, read from the first FAT copy. */
static int fat_next(struct fat16_vol *v, unsigned int c, unsigned int *next)
{
	unsigned char sec[FAT_SECTOR_BYTES];
	unsigned long off = (unsigned long) c * 2u;
	unsigned int lba = v->fat_start + (unsigned int) (off / FAT_SECTOR_BYTES);
	int rc;

	if (lba >= v->fat_start + v->fat_sectors)
		return FAT_ERR_CHAIN;
	rc = rd_sector(v, lba, sec);
	if (rc != FAT_OK)
		return rc;
	*next = rd16(sec + (off % FAT_SECTOR_BYTES));
	return FAT_OK;
}

int fat16_mount(struct fat16_vol *v, fat_read_fn read, void *ctx,
                unsigned long device_sectors)
{
	unsigned char sec[FAT_SECTOR_BYTES];
	unsigned int bps, spc, reserved, nfats, rootent, total16, spf;
	unsigned long total, root_sectors, meta, data_sectors, clusters;
	int rc;

	v->mounted = 0;
	v->read = read;
	v->ctx = ctx;
	if (read == 0 || device_sectors == 0 || device_sectors > 0xffffffu)
		return FAT_ERR_RANGE;
	v->total_sectors = (unsigned int) device_sectors;

	rc = rd_sector(v, 0, sec);
	if (rc != FAT_OK)
		return rc;
	if (sec[510] != 0x55 || sec[511] != 0xaa)
		return FAT_ERR_BPB;

	bps = rd16(sec + 11);
	spc = sec[13];
	reserved = rd16(sec + 14);
	nfats = sec[16];
	rootent = rd16(sec + 17);
	total16 = rd16(sec + 19);
	spf = rd16(sec + 22);
	if (bps != FAT_SECTOR_BYTES)
		return FAT_ERR_BPB;
	if (spc == 0 || spc > 128 || (spc & (spc - 1)) != 0)
		return FAT_ERR_BPB;
	if (reserved == 0 || nfats != 2 || spf == 0)
		return FAT_ERR_BPB;
	if (rootent == 0 || (rootent * 32u) % FAT_SECTOR_BYTES != 0)
		return FAT_ERR_BPB;

	total = total16 != 0 ? (unsigned long) total16 : rd32(sec + 32);
	if (total == 0 || total > device_sectors)
		return FAT_ERR_BPB;

	root_sectors = ((unsigned long) rootent * 32u + FAT_SECTOR_BYTES - 1) / FAT_SECTOR_BYTES;
	meta = (unsigned long) reserved + 2ul * spf + root_sectors;
	if (meta >= total)
		return FAT_ERR_BPB;
	data_sectors = total - meta;
	clusters = data_sectors / spc;

	/* FAT type is decided by the cluster count alone. */
	if (clusters < FAT16_MIN_CLUSTERS || clusters > FAT16_MAX_CLUSTERS)
		return FAT_ERR_TYPE;
	/* The FAT must hold entries 0 .. clusters+1. */
	if ((unsigned long) spf * (FAT_SECTOR_BYTES / 2) < clusters + 2)
		return FAT_ERR_BPB;

	v->fat_start = reserved;
	v->fat_sectors = spf;
	v->root_start = reserved + 2u * spf;
	v->root_sectors = (unsigned int) root_sectors;
	v->data_start = (unsigned int) meta;
	v->spc = spc;
	v->clusters = (unsigned int) clusters;
	v->total_sectors = (unsigned int) total;
	v->mounted = 1;
	return FAT_OK;
}

int fat16_name83(const char *name, unsigned char out[11])
{
	unsigned int i = 0, k = 0, dot = 0;
	unsigned int base = 0, ext = 0;

	for (i = 0; i < 11; i++)
		out[i] = ' ';
	for (i = 0; name[i] != 0; i++) {
		char c = name[i];
		if (c == '.') {
			if (dot)
				return FAT_ERR_NAME;
			dot = 1;
			k = 0;
			continue;
		}
		if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'))
			return FAT_ERR_NAME;
		if (!dot) {
			if (base == 8)
				return FAT_ERR_NAME;
			out[base++] = (unsigned char) c;
		} else {
			if (ext == 3)
				return FAT_ERR_NAME;
			out[8 + ext++] = (unsigned char) c;
		}
		k++;
	}
	if (base == 0)
		return FAT_ERR_NAME;
	return FAT_OK;
}

/* Walk the chain of a file of 'size' bytes and check it: every cluster in
 * range, no cycle, and exactly the number of clusters the size needs. */
static int chain_check(struct fat16_vol *v, unsigned int first, unsigned long size)
{
	unsigned long cbytes = (unsigned long) v->spc * FAT_SECTOR_BYTES;
	unsigned long need = (size + cbytes - 1) / cbytes;
	unsigned long len = 0;
	unsigned int c = first, n = 0;
	int rc;

	if (need == 0)
		return FAT_OK;
	for (;;) {
		if (c < 2 || c > v->clusters + 1)
			return FAT_ERR_CHAIN;
		len++;
		if (len > need)              /* longer than the file, or a cycle */
			return FAT_ERR_CHAIN;
		rc = fat_next(v, c, &n);
		if (rc != FAT_OK)
			return rc;
		if (n >= FAT_EOC_MIN)
			break;
		c = n;
	}
	if (len != need)
		return FAT_ERR_SIZE;
	return FAT_OK;
}

int fat16_open_root(struct fat16_vol *v, const char *name, struct fat16_file *f)
{
	unsigned char want[11];
	unsigned char sec[FAT_SECTOR_BYTES];
	unsigned int s, e;
	int rc;

	if (!v->mounted)
		return FAT_ERR_BPB;
	rc = fat16_name83(name, want);
	if (rc != FAT_OK)
		return rc;

	for (s = 0; s < v->root_sectors; s++) {
		rc = rd_sector(v, v->root_start + s, sec);
		if (rc != FAT_OK)
			return rc;
		for (e = 0; e < FAT_SECTOR_BYTES / 32; e++) {
			unsigned char *p = sec + 32 * e;
			if (p[0] == 0x00)
				return FAT_ERR_NOFILE;     /* end of directory */
			if (p[0] == 0xe5)
				continue;                  /* deleted */
			if (p[11] & 0x18)
				continue;                  /* volume label or subdirectory;
				                             * long-name entries carry 0x08 too */
			if (!same11(p, want))
				continue;

			f->vol = v;
			f->first = rd16(p + 26);
			f->size = rd32(p + 28);
			f->dir_lba = v->root_start + s;
			f->dir_off = 32 * e;
			f->pos = 0;
			f->cur = f->first;
			f->cur_idx = 0;

			if (f->size != 0 && (f->first < 2 || f->first > v->clusters + 1))
				return FAT_ERR_CHAIN;
			return chain_check(v, f->first, f->size);
		}
	}
	return FAT_ERR_NOFILE;
}

int fat16_read(struct fat16_file *f, unsigned char *dst, unsigned int n, unsigned int *got)
{
	struct fat16_vol *v = f->vol;
	unsigned long cbytes = (unsigned long) v->spc * FAT_SECTOR_BYTES;
	unsigned char sec[FAT_SECTOR_BYTES];
	unsigned int total = 0;
	int rc;

	*got = 0;
	while (n > 0 && f->pos < f->size) {
		unsigned long k = f->pos / cbytes;
		unsigned long within = f->pos % cbytes;
		unsigned long off = f->pos % FAT_SECTOR_BYTES;
		unsigned long lba;
		unsigned long avail;

		if (f->cur_idx > k) {          /* backwards: restart from the first cluster */
			f->cur = f->first;
			f->cur_idx = 0;
		}
		while (f->cur_idx < k) {
			unsigned int nx = 0;
			rc = fat_next(v, f->cur, &nx);
			if (rc != FAT_OK)
				return rc;
			if (nx < 2 || nx > v->clusters + 1)
				return FAT_ERR_CHAIN;
			f->cur = nx;
			f->cur_idx++;
		}

		lba = (unsigned long) v->data_start + (unsigned long) (f->cur - 2u) * v->spc +
		      within / FAT_SECTOR_BYTES;
		rc = rd_sector(v, (unsigned int) lba, sec);
		if (rc != FAT_OK)
			return rc;

		avail = FAT_SECTOR_BYTES - off;
		if (avail > n)
			avail = n;
		if (avail > f->size - f->pos)
			avail = f->size - f->pos;
		{
			unsigned long i;
			for (i = 0; i < avail; i++)
				dst[i] = sec[off + i];
		}
		dst += avail;
		f->pos += avail;
		n -= (unsigned int) avail;
		total += (unsigned int) avail;
	}
	*got = total;
	return FAT_OK;
}

int fat16_seek(struct fat16_file *f, unsigned long pos)
{
	if (pos > f->size)
		return FAT_ERR_RANGE;
	f->pos = pos;
	return FAT_OK;
}
