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
			f->is_dir = 0;
			f->dir_next = 0;
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

volatile unsigned long fat16_dir_skipped = 0;

static int name_byte_ok(unsigned char c)
{
#ifdef QDOS_VFAT
	switch (c) {
	case '!': case '#': case '$': case '%': case '&': case '\'': case '(': case ')': case '-':
	case '@': case '^': case '`': case '{': case '}': case '~':
		return 1;
	}
#endif
	return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

int fat16_dir_open(struct fat16_vol *v, struct fat16_file *f)
{
	if (!v->mounted)
		return FAT_ERR_BPB;
	f->vol = v;
	f->first = 0;
	f->size = 0;
	f->pos = 0;
	f->cur = 0;
	f->cur_idx = 0;
	f->dir_lba = 0;
	f->dir_off = 0;
	f->is_dir = 1;
	f->dir_next = 0;
	return FAT_OK;
}

int fat16_dir_next(struct fat16_file *f, struct fat16_dirent *out)
{
	struct fat16_vol *v = f->vol;
	unsigned int per_sector = FAT_SECTOR_BYTES / 32;
	unsigned int total = v->root_sectors * per_sector;
	unsigned char sec[FAT_SECTOR_BYTES];

	while (f->dir_next < total) {
		unsigned int idx = f->dir_next;
		unsigned int s = idx / per_sector;
		unsigned int e = idx % per_sector;
		unsigned char *p;
		unsigned int blen = 8, elen = 3, i, k;
		int rc;

		rc = rd_sector(v, v->root_start + s, sec);
		if (rc != FAT_OK)
			return rc;
		p = sec + 32 * e;
		f->dir_next++;

		if (p[0] == 0x00) {              /* end of directory: sticky */
			f->dir_next = total;
			return 0;
		}
		if (p[0] == 0xe5)                /* deleted */
			continue;
		if (p[11] == 0x0f)               /* long-name fragment */
			continue;
		if (p[11] & 0x18)                /* volume label or subdirectory */
			continue;

		while (blen > 0 && p[blen - 1] == ' ')
			blen--;
		while (elen > 0 && p[8 + elen - 1] == ' ')
			elen--;
		if (blen == 0) {
			fat16_dir_skipped++;
			continue;
		}
		for (i = 0; i < blen; i++)
			if (!name_byte_ok(p[i]))
				break;
		if (i != blen) {
			fat16_dir_skipped++;
			continue;
		}
		for (i = 0; i < elen; i++)
			if (!name_byte_ok(p[8 + i]))
				break;
		if (i != elen) {
			fat16_dir_skipped++;
			continue;
		}

		k = 0;
		for (i = 0; i < blen; i++)
			out->name[k++] = (char) p[i];
		if (elen != 0) {
			out->name[k++] = '.';
			for (i = 0; i < elen; i++)
				out->name[k++] = (char) p[8 + i];
		}
		while (k < 12)
			out->name[k++] = 0;
		out->size = rd32(p + 28);
		return 1;
	}
	return 0;
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

/* ======================================================================================
 * R11L: writable volume (compiled only with -DQDOS_FS_WRITE).  See fat16.h and
 * docs/R11L-FAT-WRITE-ABI.md for the ordering and crash-consistency policy.
 * ====================================================================================== */
#ifdef QDOS_FS_WRITE

#ifndef QDOS_FS_STAMP_DATE
#define QDOS_FS_STAMP_DATE 0x5d5cu   /* 2026-10-28: fixed, so that a build is reproducible */
#endif
#ifndef QDOS_FS_STAMP_TIME
#define QDOS_FS_STAMP_TIME 0x0000u
#endif

static void wr16(unsigned char *p, unsigned int v)
{
	p[0] = (unsigned char) (v & 0xff);
	p[1] = (unsigned char) ((v >> 8) & 0xff);
}

static void wr32(unsigned char *p, unsigned long v)
{
	p[0] = (unsigned char) (v & 0xff);
	p[1] = (unsigned char) ((v >> 8) & 0xff);
	p[2] = (unsigned char) ((v >> 16) & 0xff);
	p[3] = (unsigned char) ((v >> 24) & 0xff);
}

void fat16_set_writer(struct fat16_vol *v, fat_write_fn w)
{
	v->write = w;
	v->next_free = 2;
	v->sectors_written = 0;
	v->wstate = 0;
}

/* The device cannot say in advance that it is write-protected (the i386VM harness opened without
 * "rw", a read-only IDE drive).  The first operation that needs to write rewrites sector 0 with its
 * own content: harmless on a writable device, and it learns the answer before a file is created
 * or truncated.  Only a definite "write-protected" (-2) is latched; an I/O error is not. */
static int probe_write(struct fat16_vol *v)
{
	unsigned char sec[FAT_SECTOR_BYTES];
	int rc;

	if (v->write == 0)
		return FAT_ERR_WPROT;
	if (v->wstate > 0)
		return FAT_OK;
	if (v->wstate < 0)
		return FAT_ERR_WPROT;
	rc = rd_sector(v, 0, sec);
	if (rc != FAT_OK)
		return rc;
	rc = v->write(v->ctx, 0, sec);
	if (rc == -2) {
		v->wstate = -1;
		return FAT_ERR_WPROT;
	}
	if (rc != 0)
		return FAT_ERR_WIO;
	v->wstate = 1;
	v->sectors_written++;
	return FAT_OK;
}

/* Every sector write goes through here: in the volume, writer present, device accepts it. */
static int wr_sector(struct fat16_vol *v, unsigned int lba, const unsigned char *buf)
{
	int rc;
	if (lba >= v->total_sectors)
		return FAT_ERR_RANGE;
	if (v->write == 0)
		return FAT_ERR_WPROT;
	rc = v->write(v->ctx, lba, buf);
	if (rc == -2)
		return FAT_ERR_WPROT;
	if (rc != 0)
		return FAT_ERR_WIO;
	v->sectors_written++;
	return FAT_OK;
}

/* ---- FAT updates: first copy, then second copy ---------------------------------------- */

/* Write a modified FAT sector (index within the FAT) to both copies, first then second. */
static int fat_put_sector(struct fat16_vol *v, unsigned int idx, const unsigned char *sec)
{
	int rc = wr_sector(v, v->fat_start + idx, sec);
	if (rc != FAT_OK)
		return rc;
	return wr_sector(v, v->fat_start + v->fat_sectors + idx, sec);
}

/* Set FAT entry c to val in both copies. */
static int fat_set(struct fat16_vol *v, unsigned int c, unsigned int val)
{
	unsigned char sec[FAT_SECTOR_BYTES];
	unsigned long off = (unsigned long) c * 2u;
	unsigned int idx = (unsigned int) (off / FAT_SECTOR_BYTES);
	int rc;

	if (idx >= v->fat_sectors)
		return FAT_ERR_CHAIN;
	rc = rd_sector(v, v->fat_start + idx, sec);
	if (rc != FAT_OK)
		return rc;
	wr16(sec + (off % FAT_SECTOR_BYTES), val);
	return fat_put_sector(v, idx, sec);
}

/* Find a free cluster (entry 0), starting at the hint and wrapping once.  Bad (0xFFF7) and every
 * other nonzero entry count as used.  Does not mark it. */
static int fat_find_free(struct fat16_vol *v, unsigned int *out)
{
	unsigned char sec[FAT_SECTOR_BYTES];
	unsigned int start = v->next_free;
	unsigned int c, last = v->clusters + 1;
	unsigned int loaded = 0xffffffffu;
	unsigned int n;
	int rc;

	if (start < 2 || start > last)
		start = 2;
	c = start;
	for (n = 0; n < v->clusters; n++) {
		unsigned long off = (unsigned long) c * 2u;
		unsigned int idx = (unsigned int) (off / FAT_SECTOR_BYTES);
		if (idx != loaded) {
			rc = rd_sector(v, v->fat_start + idx, sec);
			if (rc != FAT_OK)
				return rc;
			loaded = idx;
		}
		if (rd16(sec + (off % FAT_SECTOR_BYTES)) == 0) {
			*out = c;
			v->next_free = (c == last) ? 2 : c + 1;
			return FAT_OK;
		}
		c = (c == last) ? 2 : c + 1;
	}
	return FAT_ERR_FULL;
}

int fat16_free_clusters(struct fat16_vol *v, unsigned long *nfree)
{
	unsigned char sec[FAT_SECTOR_BYTES];
	unsigned int c, loaded = 0xffffffffu;
	unsigned long cnt = 0;
	int rc;

	if (!v->mounted)
		return FAT_ERR_BPB;
	for (c = 2; c <= v->clusters + 1; c++) {
		unsigned long off = (unsigned long) c * 2u;
		unsigned int idx = (unsigned int) (off / FAT_SECTOR_BYTES);
		if (idx != loaded) {
			rc = rd_sector(v, v->fat_start + idx, sec);
			if (rc != FAT_OK)
				return rc;
			loaded = idx;
		}
		if (rd16(sec + (off % FAT_SECTOR_BYTES)) == 0)
			cnt++;
	}
	*nfree = cnt;
	return FAT_OK;
}

/* Free the chain starting at c, head first, one FAT sector write pair per sector touched.  The
 * walk is bounded by the cluster count, so a cycle cannot loop.  Stops (error) at an entry that is
 * not a valid link or end marker, after writing what it has freed. */
static int chain_free(struct fat16_vol *v, unsigned int c)
{
	unsigned char sec[FAT_SECTOR_BYTES];
	unsigned int loaded = 0xffffffffu;
	int dirty = 0;
	unsigned int steps = 0;
	int rc = FAT_OK, rc2;

	for (;;) {
		unsigned long off;
		unsigned int idx, nx;

		if (c < 2 || c > v->clusters + 1 || steps++ > v->clusters) {
			rc = FAT_ERR_CHAIN;
			break;
		}
		off = (unsigned long) c * 2u;
		idx = (unsigned int) (off / FAT_SECTOR_BYTES);
		if (idx != loaded) {
			if (dirty) {
				rc = fat_put_sector(v, loaded, sec);
				dirty = 0;
				if (rc != FAT_OK)
					return rc;
			}
			rc = rd_sector(v, v->fat_start + idx, sec);
			if (rc != FAT_OK)
				return rc;
			loaded = idx;
		}
		nx = rd16(sec + (off % FAT_SECTOR_BYTES));
		if (nx == 0) {                /* already free: a malformed chain */
			rc = FAT_ERR_CHAIN;
			break;
		}
		wr16(sec + (off % FAT_SECTOR_BYTES), 0);
		dirty = 1;
		if (c < v->next_free)
			v->next_free = c;
		if (nx >= FAT_EOC_MIN)
			break;
		c = nx;
	}
	if (dirty) {
		rc2 = fat_put_sector(v, loaded, sec);
		if (rc == FAT_OK)
			rc = rc2;
	}
	return rc;
}

/* ---- directory ------------------------------------------------------------------------- */

static void stamp(unsigned char *p)
{
	wr16(p + 22, QDOS_FS_STAMP_TIME);
	wr16(p + 24, QDOS_FS_STAMP_DATE);
}

/* Write size and first cluster (and the stamp) into the directory entry of f. */
static int dir_update(struct fat16_vol *v, unsigned int lba, unsigned int off,
                      unsigned int first, unsigned long size)
{
	unsigned char sec[FAT_SECTOR_BYTES];
	int rc = rd_sector(v, lba, sec);
	if (rc != FAT_OK)
		return rc;
	wr16(sec + off + 26, first);
	wr32(sec + off + 28, size);
	stamp(sec + off);
	return wr_sector(v, lba, sec);
}

int fat16_flush(struct fat16_file *f)
{
	int rc;
	if (f->is_dir || !(f->mode & FAT_O_WRITE) || !f->dirty)
		return FAT_OK;
	rc = dir_update(f->vol, (unsigned int) f->dir_lba, f->dir_off, f->first, f->size);
	if (rc == FAT_OK)
		f->dirty = 0;
	return rc;
}

/* Scan the root for name.  found: 1 with the entry's sector and offset in lba and off, and the
 * entry bytes copied to ent (32 bytes).  Always reports in freelba and freeoff the first free slot
 * (deleted, or the end marker), with havefree 0 if the directory is full.  A volume label or
 * directory with the same name is reported as FAT_ERR_EXIST. */
static int dir_find(struct fat16_vol *v, const unsigned char want[11], int *found,
                    unsigned int *lba, unsigned int *off, unsigned char *ent,
                    int *havefree, unsigned int *freelba, unsigned int *freeoff)
{
	unsigned char sec[FAT_SECTOR_BYTES];
	unsigned int s, e, i;
	int rc;

	*found = 0;
	*havefree = 0;
	for (s = 0; s < v->root_sectors; s++) {
		rc = rd_sector(v, v->root_start + s, sec);
		if (rc != FAT_OK)
			return rc;
		for (e = 0; e < FAT_SECTOR_BYTES / 32; e++) {
			unsigned char *p = sec + 32 * e;
			if (p[0] == 0x00) {
				if (!*havefree) {
					*havefree = 1;
					*freelba = v->root_start + s;
					*freeoff = 32 * e;
				}
				return FAT_OK;       /* end of directory */
			}
			if (p[0] == 0xe5) {
				if (!*havefree) {
					*havefree = 1;
					*freelba = v->root_start + s;
					*freeoff = 32 * e;
				}
				continue;
			}
			if (p[11] == 0x0f)
				continue;
			if (!same11(p, want))
				continue;
			if (p[11] & 0x18)
				return FAT_ERR_EXIST;
			*found = 1;
			*lba = v->root_start + s;
			*off = 32 * e;
			for (i = 0; i < 32; i++)
				ent[i] = p[i];
			return FAT_OK;
		}
	}
	return FAT_OK;
}

int fat16_lookup(struct fat16_vol *v, const char *name, unsigned int *lba, unsigned int *off)
{
	unsigned char want[11], ent[32];
	unsigned int flba = 0, foff = 0;
	int found, havefree, rc;

	if (!v->mounted)
		return FAT_ERR_BPB;
	rc = fat16_name83(name, want);
	if (rc != FAT_OK)
		return rc;
	rc = dir_find(v, want, &found, lba, off, ent, &havefree, &flba, &foff);
	if (rc != FAT_OK)
		return rc;
	return found ? FAT_OK : FAT_ERR_NOFILE;
}

int fat16_open_ex(struct fat16_vol *v, const char *name, unsigned int flags, struct fat16_file *f)
{
	unsigned char want[11], ent[32], sec[FAT_SECTOR_BYTES];
	unsigned int lba = 0, off = 0, flba = 0, foff = 0, i;
	int found, havefree, rc;

	if (!v->mounted)
		return FAT_ERR_BPB;
	if (!(flags & (FAT_O_READ | FAT_O_WRITE)))
		return FAT_ERR_WPROT;
	if (!(flags & FAT_O_WRITE)) {
		rc = fat16_open_root(v, name, f);
		f->mode = FAT_O_READ;
		f->dirty = 0;
		return rc;
	}
	rc = probe_write(v);
	if (rc != FAT_OK)
		return rc;
	rc = fat16_name83(name, want);
	if (rc != FAT_OK)
		return rc;
	rc = dir_find(v, want, &found, &lba, &off, ent, &havefree, &flba, &foff);
	if (rc != FAT_OK)
		return rc;

	f->vol = v;
	f->is_dir = 0;
	f->dir_next = 0;
	f->pos = 0;
	f->cur_idx = 0;
	f->mode = flags;
	f->dirty = 0;

	if (found) {
		if ((flags & FAT_O_CREATE) && (flags & FAT_O_EXCL))
			return FAT_ERR_EXIST;
		f->first = rd16(ent + 26);
		f->size = rd32(ent + 28);
		f->cur = f->first;
		f->dir_lba = lba;
		f->dir_off = off;
		if (f->size != 0 && (f->first < 2 || f->first > v->clusters + 1))
			return FAT_ERR_CHAIN;
		rc = chain_check(v, f->first, f->size);
		if (rc != FAT_OK)
			return rc;
		if (flags & FAT_O_TRUNC)
			return fat16_truncate(f, 0);
		return FAT_OK;
	}
	if (!(flags & FAT_O_CREATE))
		return FAT_ERR_NOFILE;
	if (!havefree)
		return FAT_ERR_DIRFULL;

	/* New entry: name, archive attribute, stamp, no cluster, size 0 - one sector write. */
	rc = rd_sector(v, flba, sec);
	if (rc != FAT_OK)
		return rc;
	for (i = 0; i < 32; i++)
		sec[foff + i] = 0;
	for (i = 0; i < 11; i++)
		sec[foff + i] = want[i];
	sec[foff + 11] = 0x20;
	stamp(sec + foff);
	rc = wr_sector(v, flba, sec);
	if (rc != FAT_OK)
		return rc;
	f->first = 0;
	f->size = 0;
	f->cur = 0;
	f->dir_lba = flba;
	f->dir_off = foff;
	return FAT_OK;
}

/* ---- cluster location ------------------------------------------------------------------- */

/* Position the cursor on cluster index k of an existing chain (k < clusters in the file). */
static int seek_cluster(struct fat16_file *f, unsigned long k)
{
	struct fat16_vol *v = f->vol;
	int rc;

	if (f->first == 0)
		return FAT_ERR_CHAIN;
	if (f->cur_idx > k || f->cur < 2) {
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
	return FAT_OK;
}

/* Append a cluster to the file: choose it, write the data sector of its first sector first (the
 * cluster is still free in the FAT, so a cut here loses nothing), then FAT end marker, then the
 * predecessor link or the first-cluster field.  secbuf is the 512-byte first sector of the new
 * cluster.  On success the cursor is on the new cluster (index k).  The directory entry is
 * written by the caller. */
static int append_cluster(struct fat16_file *f, unsigned long k, const unsigned char *secbuf)
{
	struct fat16_vol *v = f->vol;
	unsigned int nc, prev = 0;
	int rc;

	if (k > 0) {
		rc = seek_cluster(f, k - 1);
		if (rc != FAT_OK)
			return rc;
		prev = f->cur;
	}
	rc = fat_find_free(v, &nc);
	if (rc != FAT_OK)
		return rc;
	rc = wr_sector(v, v->data_start + (unsigned int) (nc - 2u) * v->spc, secbuf);
	if (rc != FAT_OK)
		return rc;
	rc = fat_set(v, nc, 0xffff);
	if (rc != FAT_OK)
		return rc;
	if (k > 0) {
		rc = fat_set(v, prev, nc);
		if (rc != FAT_OK)
			return rc;
	} else {
		f->first = nc;
	}
	f->cur = nc;
	f->cur_idx = k;
	return FAT_OK;
}

int fat16_write(struct fat16_file *f, const unsigned char *src, unsigned int n, unsigned int *put)
{
	struct fat16_vol *v = f->vol;
	unsigned long cbytes = (unsigned long) v->spc * FAT_SECTOR_BYTES;
	unsigned char sec[FAT_SECTOR_BYTES];
	unsigned int total = 0;
	int rc = FAT_OK;

	*put = 0;
	if (f->is_dir || !(f->mode & FAT_O_WRITE))
		return FAT_ERR_WPROT;
	if (v->write == 0)
		return FAT_ERR_WPROT;
	if (f->mode & FAT_O_APPEND)
		f->pos = f->size;
	if (f->pos > f->size)
		return FAT_ERR_RANGE;

	while (n > 0) {
		unsigned long k = f->pos / cbytes;
		unsigned long within = f->pos % cbytes;
		unsigned long off = f->pos % FAT_SECTOR_BYTES;
		unsigned long have = (f->size + cbytes - 1) / cbytes;   /* clusters in the file now */
		unsigned long take = FAT_SECTOR_BYTES - off;
		unsigned long i;
		unsigned long sec_start = f->pos - off;
		int newc = 0;
		unsigned long newsize;

		if (take > n)
			take = n;
		if (k >= 0x10000ul) {
			rc = FAT_ERR_FULL;
			break;
		}
		/* Build the sector image: whole-sector overwrite needs no read; a sector that starts at or
		 * past the end of the file starts from zeros; otherwise read-modify-write. */
		if (take == FAT_SECTOR_BYTES) {
			for (i = 0; i < FAT_SECTOR_BYTES; i++)
				sec[i] = src[i];
		} else {
			if (k < have && sec_start < f->size) {
				rc = seek_cluster(f, k);
				if (rc != FAT_OK)
					break;
				rc = rd_sector(v, v->data_start + (unsigned int) (f->cur - 2u) * v->spc +
				               (unsigned int) (within / FAT_SECTOR_BYTES), sec);
				if (rc != FAT_OK)
					break;
			} else {
				for (i = 0; i < FAT_SECTOR_BYTES; i++)
					sec[i] = 0;
			}
			for (i = 0; i < take; i++)
				sec[off + i] = src[i];
		}

		if (k >= have) {
			/* Writing into a cluster the file does not have yet: only the first sector of it can
			 * be reached here (pos is at the end of the file, which is cluster-aligned). */
			if (within != 0 || k != have) {
				rc = FAT_ERR_RANGE;
				break;
			}
			rc = append_cluster(f, k, sec);
			if (rc != FAT_OK)
				break;
			newc = 1;
		} else {
			rc = seek_cluster(f, k);
			if (rc != FAT_OK)
				break;
			rc = wr_sector(v, v->data_start + (unsigned int) (f->cur - 2u) * v->spc +
			               (unsigned int) (within / FAT_SECTOR_BYTES), sec);
			if (rc != FAT_OK)
				break;
		}

		src += take;
		n -= (unsigned int) take;
		total += (unsigned int) take;
		f->pos += take;
		newsize = f->size;
		if (f->pos > newsize)
			newsize = f->pos;
		if (newsize != f->size || newc)
			f->dirty = 1;
		f->size = newsize;
		if (newc) {
			/* the FAT now shows the new cluster: bring the directory entry in line at once so
			 * that the window in which chain and size disagree is two sector writes wide */
			rc = fat16_flush(f);
			if (rc != FAT_OK)
				break;
		}
	}
	*put = total;
	if (total != 0)
		return FAT_OK;               /* short write: the error is reported by the next call */
	return rc;
}

int fat16_truncate(struct fat16_file *f, unsigned long size)
{
	struct fat16_vol *v = f->vol;
	unsigned long cbytes = (unsigned long) v->spc * FAT_SECTOR_BYTES;
	int rc;

	if (f->is_dir || !(f->mode & FAT_O_WRITE) || v->write == 0)
		return FAT_ERR_WPROT;
	if (size == f->size)
		return FAT_OK;

	if (size > f->size) {
		unsigned char zeros[64];
		unsigned long savepos = f->pos;
		unsigned int i;
		unsigned int saved_mode = f->mode;

		for (i = 0; i < sizeof zeros; i++)
			zeros[i] = 0;
		f->mode &= ~FAT_O_APPEND;
		f->pos = f->size;
		while (f->size < size) {
			unsigned long want = size - f->size;
			unsigned int put = 0;
			if (want > sizeof zeros)
				want = sizeof zeros;
			rc = fat16_write(f, zeros, (unsigned int) want, &put);
			if (rc != FAT_OK || put == 0) {
				f->pos = savepos;
				f->mode = saved_mode;
				(void) fat16_flush(f);
				return rc != FAT_OK ? rc : FAT_ERR_WIO;
			}
		}
		f->pos = savepos;
		f->mode = saved_mode;
		return fat16_flush(f);
	}

	/* shrink */
	{
		unsigned long need = (size + cbytes - 1) / cbytes;
		unsigned int tail = 0, keep_last = 0, oldfirst = f->first;

		if (need == 0) {
			f->first = 0;
			f->cur = 0;
			f->cur_idx = 0;
			f->size = 0;
			rc = dir_update(v, (unsigned int) f->dir_lba, f->dir_off, 0, 0);
			if (rc != FAT_OK)
				return rc;
			f->dirty = 0;
			f->pos = 0;
			if (oldfirst != 0)
				return chain_free(v, oldfirst);
			return FAT_OK;
		}
		/* size first (chain longer than size = a state the checker repairs), then cut the chain,
		 * then free the tail */
		rc = seek_cluster(f, need - 1);
		if (rc != FAT_OK)
			return rc;
		keep_last = f->cur;
		rc = fat_next(v, keep_last, &tail);
		if (rc != FAT_OK)
			return rc;
		f->size = size;
		rc = dir_update(v, (unsigned int) f->dir_lba, f->dir_off, f->first, size);
		if (rc != FAT_OK)
			return rc;
		f->dirty = 0;
		if (f->pos > size)
			f->pos = size;
		if (tail < FAT_EOC_MIN) {
			rc = fat_set(v, keep_last, 0xffff);
			if (rc != FAT_OK)
				return rc;
			return chain_free(v, tail);
		}
		return FAT_OK;
	}
}

int fat16_remove(struct fat16_vol *v, const char *name)
{
	unsigned char want[11], ent[32], sec[FAT_SECTOR_BYTES];
	unsigned int lba = 0, off = 0, flba = 0, foff = 0, first;
	unsigned long size;
	int found, havefree, rc;

	if (!v->mounted)
		return FAT_ERR_BPB;
	rc = probe_write(v);
	if (rc != FAT_OK)
		return rc;
	rc = fat16_name83(name, want);
	if (rc != FAT_OK)
		return rc;
	rc = dir_find(v, want, &found, &lba, &off, ent, &havefree, &flba, &foff);
	if (rc != FAT_OK)
		return rc;
	if (!found)
		return FAT_ERR_NOFILE;
	first = rd16(ent + 26);
	size = rd32(ent + 28);
	if (size != 0 && (first < 2 || first > v->clusters + 1))
		return FAT_ERR_CHAIN;
	rc = chain_check(v, first, size);
	if (rc != FAT_OK)
		return rc;
	rc = rd_sector(v, lba, sec);
	if (rc != FAT_OK)
		return rc;
	sec[off] = 0xe5;
	rc = wr_sector(v, lba, sec);
	if (rc != FAT_OK)
		return rc;
	if (size == 0)
		return FAT_OK;
	return chain_free(v, first);
}

int fat16_rename(struct fat16_vol *v, const char *from, const char *to)
{
	unsigned char wf[11], wt[11], ent[32], sec[FAT_SECTOR_BYTES];
	unsigned int lba = 0, off = 0, flba = 0, foff = 0, l2 = 0, o2 = 0, i;
	int found, havefree, rc;

	if (!v->mounted)
		return FAT_ERR_BPB;
	rc = probe_write(v);
	if (rc != FAT_OK)
		return rc;
	rc = fat16_name83(from, wf);
	if (rc != FAT_OK)
		return rc;
	rc = fat16_name83(to, wt);
	if (rc != FAT_OK)
		return rc;
	rc = dir_find(v, wf, &found, &lba, &off, ent, &havefree, &flba, &foff);
	if (rc != FAT_OK)
		return rc;
	if (!found)
		return FAT_ERR_NOFILE;
	if (same11(wf, wt))
		return FAT_OK;
	rc = dir_find(v, wt, &found, &l2, &o2, ent, &havefree, &flba, &foff);
	if (rc != FAT_OK)
		return rc;
	if (found)
		return FAT_ERR_EXIST;
	rc = rd_sector(v, lba, sec);
	if (rc != FAT_OK)
		return rc;
	for (i = 0; i < 11; i++)
		sec[off + i] = wt[i];
	return wr_sector(v, lba, sec);
}

/* ======================================================================================
 * R11M: subdirectories and paths (same -DQDOS_FS_WRITE build).  See docs/R11M-FAT-PATH-ABI.md.
 *
 * A directory is addressed by its first cluster (0 = the root's fixed region).  Entries are
 * addressed by index: entry i is in sector i/16 of the directory, offset 32*(i%16).  Every scan is
 * bounded: a chain is followed at most 'clusters' steps, so a loop or a damaged chain is an error,
 * never a hang.
 * ====================================================================================== */

#define DIR_PER_SEC      (FAT_SECTOR_BYTES / 32)
#define DIR_MAX_ENTRIES  65536ul     /* FAT limit for one directory */

struct dircur {                      /* position cache for walking a directory's cluster chain */
	unsigned int cl;                 /* cluster number of chain index k (0 = not started) */
	unsigned long k;
};

/* LBA of sector sidx of directory dir.  FAT_OK; 1 if the directory has no such sector; or an error. */
static int dir_sector(struct fat16_vol *v, unsigned int dir, struct dircur *cur, unsigned long sidx,
                      unsigned int *lba)
{
	unsigned long k;
	int rc;

	if (dir == 0) {
		if (sidx >= v->root_sectors)
			return 1;
		*lba = v->root_start + (unsigned int) sidx;
		return FAT_OK;
	}
	if (dir < 2 || dir > v->clusters + 1)
		return FAT_ERR_CHAIN;
	k = sidx / v->spc;
	if (cur->cl < 2 || cur->k > k) {
		cur->cl = dir;
		cur->k = 0;
	}
	while (cur->k < k) {
		unsigned int nx = 0;
		rc = fat_next(v, cur->cl, &nx);
		if (rc != FAT_OK)
			return rc;
		if (nx >= FAT_EOC_MIN)
			return 1;
		if (nx < 2 || nx > v->clusters + 1)
			return FAT_ERR_CHAIN;
		cur->cl = nx;
		cur->k++;
		if (cur->k > v->clusters)
			return FAT_ERR_CHAIN;    /* longer than the volume: a cycle */
	}
	*lba = v->data_start + (unsigned int) (cur->cl - 2u) * v->spc + (unsigned int) (sidx % v->spc);
	return FAT_OK;
}

/* Read the sector that holds entry idx; *eoff is the entry's byte offset in it.  1 = past the end. */
static int dir_read_entry_sec(struct fat16_vol *v, unsigned int dir, struct dircur *cur,
                              unsigned long idx, unsigned char *sec, unsigned int *lba, unsigned int *eoff)
{
	int rc = dir_sector(v, dir, cur, idx / DIR_PER_SEC, lba);
	if (rc != FAT_OK)
		return rc;
	rc = rd_sector(v, *lba, sec);
	if (rc != FAT_OK)
		return rc;
	*eoff = 32u * (unsigned int) (idx % DIR_PER_SEC);
	return FAT_OK;
}

/* Number of entry slots the directory has now. */
static int dir_capacity(struct fat16_vol *v, unsigned int dir, unsigned long *cap)
{
	struct dircur cur;
	unsigned int lba = 0;
	unsigned long s = 0;
	int rc;

	if (dir == 0) {
		*cap = (unsigned long) v->root_sectors * DIR_PER_SEC;
		return FAT_OK;
	}
	cur.cl = 0;
	cur.k = 0;
	/* one probe per cluster is enough */
	for (;;) {
		rc = dir_sector(v, dir, &cur, s, &lba);
		if (rc < 0)
			return rc;
		if (rc == 1)
			break;
		s += v->spc;
	}
	*cap = s * DIR_PER_SEC;
	return FAT_OK;
}

struct dfind {
	int found;
	unsigned long idx;               /* entry index of the match */
	unsigned int lba, off;
	unsigned char ent[32];
#ifdef QDOS_VFAT
	unsigned int lfn_n;              /* long-name entries that precede the short entry and are trusted (0 = none) */
#endif
};

#ifdef QDOS_VFAT
static int dir_search2(struct fat16_vol *v, unsigned int dir, const unsigned char *want,
                       const char *lw, unsigned int lwlen, struct dfind *r);

/* Search directory dir for the 11-byte name (VFAT build: also reports the trusted long-name chain). */
static int dir_search(struct fat16_vol *v, unsigned int dir, const unsigned char *want, struct dfind *r)
{
	return dir_search2(v, dir, want, 0, 0, r);
}
#else
/* Search directory dir for the 11-byte name.  Stops at the end marker or the end of the chain. */
static int dir_search(struct fat16_vol *v, unsigned int dir, const unsigned char *want, struct dfind *r)
{
	unsigned char sec[FAT_SECTOR_BYTES];
	struct dircur cur;
	unsigned long sidx = 0;
	unsigned int lba = 0, e, i;
	int rc;

	r->found = 0;
	cur.cl = 0;
	cur.k = 0;
	for (;;) {
		rc = dir_sector(v, dir, &cur, sidx, &lba);
		if (rc < 0)
			return rc;
		if (rc == 1)
			return FAT_OK;               /* end of the chain: not found */
		rc = rd_sector(v, lba, sec);
		if (rc != FAT_OK)
			return rc;
		for (e = 0; e < DIR_PER_SEC; e++) {
			unsigned char *p = sec + 32 * e;
			if (p[0] == 0x00)
				return FAT_OK;           /* end marker */
			if (p[0] == 0xe5 || p[11] == 0x0f)
				continue;
			if ((p[11] & 0x08) && !(p[11] & 0x10))
				continue;                /* volume label */
			if (!same11(p, want))
				continue;
			r->found = 1;
			r->idx = sidx * DIR_PER_SEC + e;
			r->lba = lba;
			r->off = 32 * e;
			for (i = 0; i < 32; i++)
				r->ent[i] = p[i];
			return FAT_OK;
		}
		sidx++;
	}
}
#endif

/* Write a directory entry image at entry index idx (the slot must exist). */
static int dir_put_entry(struct fat16_vol *v, unsigned int dir, unsigned long idx, const unsigned char *ent)
{
	unsigned char sec[FAT_SECTOR_BYTES];
	struct dircur cur;
	unsigned int lba = 0, eoff = 0, i;
	int rc;

	cur.cl = 0;
	cur.k = 0;
	rc = dir_read_entry_sec(v, dir, &cur, idx, sec, &lba, &eoff);
	if (rc != FAT_OK)
		return rc == 1 ? FAT_ERR_CHAIN : rc;
	for (i = 0; i < 32; i++)
		sec[eoff + i] = ent[i];
	return wr_sector(v, lba, sec);
}

/* Zero every sector of cluster c. */
static int cluster_zero(struct fat16_vol *v, unsigned int c)
{
	unsigned char z[FAT_SECTOR_BYTES];
	unsigned int i;
	int rc;

	for (i = 0; i < FAT_SECTOR_BYTES; i++)
		z[i] = 0;
	for (i = 0; i < v->spc; i++) {
		rc = wr_sector(v, v->data_start + (unsigned int) (c - 2u) * v->spc + i, z);
		if (rc != FAT_OK)
			return rc;
	}
	return FAT_OK;
}

/* Append one zeroed cluster to subdirectory dir.  The new cluster is written while still free, then
 * marked end-of-chain, then linked (same order as file growth). */
static int dir_grow(struct fat16_vol *v, unsigned int dir)
{
	unsigned int c = dir, nx = 0, nc;
	unsigned long steps = 0, cap = 0;
	int rc;

	if (dir == 0)
		return FAT_ERR_DIRFULL;
	rc = dir_capacity(v, dir, &cap);
	if (rc != FAT_OK)
		return rc;
	if (cap + (unsigned long) v->spc * DIR_PER_SEC > DIR_MAX_ENTRIES)
		return FAT_ERR_DIRFULL;
	for (;;) {                           /* find the last cluster */
		rc = fat_next(v, c, &nx);
		if (rc != FAT_OK)
			return rc;
		if (nx >= FAT_EOC_MIN)
			break;
		if (nx < 2 || nx > v->clusters + 1 || steps++ > v->clusters)
			return FAT_ERR_CHAIN;
		c = nx;
	}
	rc = fat_find_free(v, &nc);
	if (rc != FAT_OK)
		return rc;
	rc = cluster_zero(v, nc);
	if (rc != FAT_OK)
		return rc;
	rc = fat_set(v, nc, 0xffff);
	if (rc != FAT_OK)
		return rc;
	return fat_set(v, c, nc);
}

/* Find (growing a subdirectory if needed) a run of n free entry slots.  *idx is the first. */
static int dir_alloc_run(struct fat16_vol *v, unsigned int dir, unsigned int n, unsigned long *idx)
{
	unsigned char sec[FAT_SECTOR_BYTES];
	struct dircur cur;
	unsigned long sidx = 0, run_start = 0, cap = 0;
	unsigned int run_len = 0, lba = 0, e;
	int rc, have_end = 0;

	cur.cl = 0;
	cur.k = 0;
	for (;;) {
		rc = dir_sector(v, dir, &cur, sidx, &lba);
		if (rc < 0)
			return rc;
		if (rc == 1)
			break;                       /* chain ends: cap = sidx * 16 */
		rc = rd_sector(v, lba, sec);
		if (rc != FAT_OK)
			return rc;
		for (e = 0; e < DIR_PER_SEC && !have_end; e++) {
			unsigned char b = sec[32 * e];
			if (b == 0x00) {
				if (run_len == 0)
					run_start = sidx * DIR_PER_SEC + e;
				have_end = 1;            /* everything from here on is free */
			} else if (b == 0xe5) {
				if (run_len == 0)
					run_start = sidx * DIR_PER_SEC + e;
				run_len++;
				if (run_len == n) {
					*idx = run_start;
					return FAT_OK;
				}
			} else {
				run_len = 0;
			}
		}
		if (have_end)
			break;
		sidx++;
	}
	if (have_end && run_start + n <= (sidx + 1) * DIR_PER_SEC) {
		*idx = run_start;                /* fits in the sector that holds the end marker */
		return FAT_OK;
	}
	if (!have_end && run_len == 0)
		run_start = sidx * DIR_PER_SEC;  /* first slot past the end of the chain */
	rc = dir_capacity(v, dir, &cap);
	if (rc != FAT_OK)
		return rc;
	while (run_start + n > cap) {
		if (dir == 0)
			return FAT_ERR_DIRFULL;
		rc = dir_grow(v, dir);
		if (rc != FAT_OK)
			return rc;
		cap += (unsigned long) v->spc * DIR_PER_SEC;
	}
	*idx = run_start;
	return FAT_OK;
}

/* ---- names and paths ------------------------------------------------------------------- */

static int ci_upper(int c)
{
	return (c >= 'a' && c <= 'z') ? c - 32 : c;
}

#ifdef QDOS_VFAT
/* Characters of a short (8.3) name: upper-case letters, digits and ! # $ % & ' ( ) - @ ^ _ ` { } ~ */
static int vf_short_char(int c)
{
	if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
		return 1;
	switch (c) {
	case '!': case '#': case '$': case '%': case '&': case '\'': case '(': case ')': case '-':
	case '@': case '^': case '_': case '`': case '{': case '}': case '~':
		return 1;
	}
	return 0;
}
#endif

/* One path component s[0..n) to the 11-byte form.  FAT_ERR_NAME if it is not a valid 8.3 name. */
static int comp83(const char *s, unsigned int n, unsigned int pf, unsigned char out[11])
{
	unsigned int i, base = 0, ext = 0, dot = 0;

	for (i = 0; i < 11; i++)
		out[i] = ' ';
	for (i = 0; i < n; i++) {
		int c = (unsigned char) s[i];
		if (pf & FATP_CI)
			c = ci_upper(c);
		if (c == '.') {
			if (dot)
				return FAT_ERR_NAME;
			dot = 1;
			continue;
		}
#ifdef QDOS_VFAT
		if (!vf_short_char(c))
			return FAT_ERR_NAME;
#else
		if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'))
			return FAT_ERR_NAME;
#endif
		if (!dot) {
			if (base == 8)
				return FAT_ERR_NAME;
			out[base++] = (unsigned char) c;
		} else {
			if (ext == 3)
				return FAT_ERR_NAME;
			out[8 + ext++] = (unsigned char) c;
		}
	}
	if (base == 0)
		return FAT_ERR_NAME;
	return FAT_OK;
}

/* The parent of directory dir, from its ".." entry (entry 1).  The root is its own parent. */
static int dir_parent(struct fat16_vol *v, unsigned int dir, unsigned int *parent)
{
	unsigned char sec[FAT_SECTOR_BYTES];
	struct dircur cur;
	unsigned int lba = 0, eoff = 0, p, i;
	int rc;

	if (dir == 0) {
		*parent = 0;
		return FAT_OK;
	}
	cur.cl = 0;
	cur.k = 0;
	rc = dir_read_entry_sec(v, dir, &cur, 1, sec, &lba, &eoff);
	if (rc != FAT_OK)
		return rc == 1 ? FAT_ERR_CHAIN : rc;
	if (sec[eoff] != '.' || sec[eoff + 1] != '.')
		return FAT_ERR_CHAIN;
	for (i = 2; i < 11; i++)
		if (sec[eoff + i] != ' ')
			return FAT_ERR_CHAIN;
	if ((sec[eoff + 11] & 0x10) == 0)
		return FAT_ERR_CHAIN;
	p = rd16(sec + eoff + 26);
	if (p != 0 && (p < 2 || p > v->clusters + 1))
		return FAT_ERR_CHAIN;
	*parent = p;
	return FAT_OK;
}

#ifdef QDOS_VFAT
/* ======================================================================================
 * R11M Stage 4: VFAT long file names.  Policy and limits: docs/R11M-VFAT-LIMITATIONS.md.
 *
 * A directory entry's long name is a chain of 0x0F-attribute entries just before its short entry,
 * highest ordinal (flag 0x40) first, ordinal 1 last.  Each carries 13 UTF-16 units and the checksum of
 * the short name it belongs to.  A chain is trusted only when it is complete (ordinals n..1 with 0x40 on
 * the first), every entry has type 0, first cluster 0 and the same checksum, and that checksum equals the
 * short entry's.  A chain that is incomplete, out of order, orphaned or mismatched is never used: the
 * short entry is then seen without a long name and the chain entries are left untouched.
 * ====================================================================================== */

struct lfnc {                        /* collector: the long-name chain being read, entry by entry, in directory order */
	unsigned char active;            /* a chain has started and is intact so far */
	unsigned char total;             /* ordinal of its first entry = number of entries */
	unsigned char expect;            /* ordinal the next entry must carry; 0 = complete, waiting for the short entry */
	unsigned char sum;               /* checksum carried by every entry */
	unsigned char term;              /* the terminator was seen in the first entry */
	unsigned char ascii;             /* every unit so far is in the supported subset */
	unsigned int len;                /* length in UTF-16 units */
	char s[FAT_LFN_MAX + 1];
};

struct vplan {                       /* how a new name will be stored */
	unsigned char alias[11];
	unsigned int nlfn;               /* long-name entries needed (0: short entry only) */
};

static const unsigned char lfn_off[13] = { 1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30 };

static unsigned int vf_sum(const unsigned char *e)
{
	unsigned int s = 0, i;
	for (i = 0; i < 11; i++)
		s = (((s & 1u) << 7) + (s >> 1) + e[i]) & 0xffu;
	return s;
}

/* A unit of the supported subset: printable ASCII except \ / : * ? " < > | */
static int vf_char_ok(unsigned int u)
{
	if (u < 0x20u || u > 0x7eu)
		return 0;
	switch (u) {
	case '\\': case '/': case ':': case '*': case '?': case '"': case '<': case '>': case '|':
		return 0;
	}
	return 1;
}

/* A name a caller may create: 1..FAT_LFN_MAX supported characters, not starting with a space or a dot, not ending in a
 * space or a dot. */
static int vf_lname_ok(const char *s, unsigned int n)
{
	unsigned int i;
	if (n == 0 || n > FAT_LFN_MAX)
		return 0;
	for (i = 0; i < n; i++)
		if (!vf_char_ok((unsigned char) s[i]))
			return 0;
	if (s[0] == ' ' || s[0] == '.' || s[n - 1] == ' ' || s[n - 1] == '.')
		return 0;
	return 1;
}

static int vf_eq(const char *a, unsigned int an, const char *b, unsigned int bn)
{
	unsigned int i;
	if (an != bn)
		return 0;
	for (i = 0; i < an; i++)
		if (ci_upper((unsigned char) a[i]) != ci_upper((unsigned char) b[i]))
			return 0;
	return 1;
}

/* Feed one long-name entry (attribute 0x0F, not deleted) to the collector.  A violation drops the chain. */
static void lfnc_feed(struct lfnc *c, const unsigned char *p)
{
	unsigned int ord = p[0], k = ord & 0x3fu, j;

	if (p[12] != 0 || p[26] != 0 || p[27] != 0 || (ord & 0x80u) != 0 || k < 1 || k > 20) {
		c->active = 0;
		return;
	}
	if (ord & 0x40u) {                   /* the first entry of a chain (highest ordinal) */
		c->active = 1;
		c->total = (unsigned char) k;
		c->expect = (unsigned char) k;
		c->sum = p[13];
		c->term = 0;
		c->ascii = 1;
		c->len = k * 13u;
	} else if (!c->active || c->expect != k || p[13] != c->sum) {
		c->active = 0;
		return;
	}
	for (j = 0; j < 13; j++) {
		unsigned int u = (unsigned int) p[lfn_off[j]] | ((unsigned int) p[lfn_off[j] + 1] << 8);
		unsigned int pos = (k - 1u) * 13u + j;

		if (k == c->total && c->term) {
			if (u != 0xffffu) {          /* damaged padding: not trusted */
				c->active = 0;
				return;
			}
			continue;
		}
		if (k == c->total && u == 0) {
			c->term = 1;
			c->len = pos;
			if (pos <= (k - 1u) * 13u) { /* the first entry would carry no characters */
				c->active = 0;
				return;
			}
			continue;
		}
		if (!vf_char_ok(u))
			c->ascii = 0;
		else if (pos < FAT_LFN_MAX)
			c->s[pos] = (char) u;
	}
	c->expect = (unsigned char) (k - 1u);
}

/* Is the collected chain complete and does it belong to the short entry p?  (Then c->total is its length.) */
static int lfnc_ok(const struct lfnc *c, const unsigned char *p)
{
	return c->active && c->expect == 0 && c->sum == vf_sum(p);
}

/* Is it also representable (then c->s is the NUL-terminated name)? */
static int lfnc_name(struct lfnc *c)
{
	if (!c->ascii || c->len < 1 || c->len > FAT_LFN_MAX)
		return 0;
	c->s[c->len] = 0;
	return 1;
}

/* Search directory dir.  want != 0: the 11-byte short name.  lw != 0: a long name, compared case-insensitively
 * with every trusted representable chain.  r->lfn_n is the trusted chain length of the match. */
static int dir_search2(struct fat16_vol *v, unsigned int dir, const unsigned char *want,
                       const char *lw, unsigned int lwlen, struct dfind *r)
{
	unsigned char sec[FAT_SECTOR_BYTES];
	struct dircur cur;
	struct lfnc lc;
	unsigned long sidx = 0;
	unsigned int lba = 0, e, i;
	int rc;

	r->found = 0;
	r->lfn_n = 0;
	lc.active = 0;
	cur.cl = 0;
	cur.k = 0;
	for (;;) {
		rc = dir_sector(v, dir, &cur, sidx, &lba);
		if (rc < 0)
			return rc;
		if (rc == 1)
			return FAT_OK;               /* end of the chain: not found */
		rc = rd_sector(v, lba, sec);
		if (rc != FAT_OK)
			return rc;
		for (e = 0; e < DIR_PER_SEC; e++) {
			unsigned char *p = sec + 32 * e;
			int hit = 0, chain;

			if (p[0] == 0x00)
				return FAT_OK;           /* end marker */
			if (p[0] == 0xe5) {
				lc.active = 0;
				continue;
			}
			if (p[11] == 0x0f) {
				lfnc_feed(&lc, p);
				continue;
			}
			chain = lfnc_ok(&lc, p);
			lc.active = 0;               /* a chain belongs to the next short entry only */
			if ((p[11] & 0x08) && !(p[11] & 0x10))
				continue;                /* volume label */
			if (want && same11(p, want))
				hit = 1;
			else if (lw && chain && lfnc_name(&lc) && vf_eq(lc.s, lc.len, lw, lwlen))
				hit = 1;
			if (hit) {
				for (i = 0; i < 32; i++)
					r->ent[i] = p[i];
				r->idx = sidx * DIR_PER_SEC + e;
				r->lba = lba;
				r->off = 32 * e;
				r->lfn_n = chain ? lc.total : 0;
				r->found = 1;
				return FAT_OK;
			}
		}
		sidx++;
	}
}

/* Look up the last component of a resolved path: the short name first, then the long name. */
static int path_find(struct fat16_vol *v, unsigned int dir, const unsigned char *nm, int short_ok,
                     const char *lname, unsigned int llen, struct dfind *r)
{
	int rc;
	if (short_ok) {
		rc = dir_search2(v, dir, nm, 0, 0, r);
		if (rc != FAT_OK || r->found)
			return rc;
	}
	return dir_search2(v, dir, 0, lname, llen, r);
}

/* One path component: its 8.3 form if it has one, and in any case a copy (the long-name candidate).
 * Names that are neither a valid short name nor a supported long name are FAT_ERR_NAME; so is any
 * long name in a legacy (pf == 0, case-sensitive) path. */
static int vf_comp(const char *s, unsigned int n, unsigned int pf, unsigned char nm[11], int *sok, char *lname)
{
	unsigned int i;
	int rc = comp83(s, n, pf, nm);

	if (rc == FAT_OK) {
		*sok = 1;
	} else {
		*sok = 0;
		if (!(pf & FATP_CI) || !vf_lname_ok(s, n))
			return FAT_ERR_NAME;
	}
	if (n > FAT_LFN_MAX)
		return FAT_ERR_NAME;
	for (i = 0; i < n; i++)
		lname[i] = s[i];
	lname[n] = 0;
	return FAT_OK;
}

/* Generate the 8.3 alias of a long name: BASE~N.EXT.  BASE is the first characters of the name before its last dot
 * (spaces and dots dropped, letters upper-cased, other unsupported characters replaced by '_'), at most 8 - 1 - digits
 * long; EXT the first three characters after the last dot.  N is the smallest number >= 1 that makes the alias
 * unused in the directory (FAT_ERR_EXIST beyond 999). */
static int vf_alias(struct fat16_vol *v, unsigned int dir, const char *ln, unsigned int n, unsigned char out[11])
{
	char base[8], ext[3];
	unsigned int nb = 0, ne = 0, ld = n, i, nn;
	struct dfind r;
	int rc;

	for (i = 0; i < n; i++)
		if (ln[i] == '.')
			ld = i;
	for (i = 0; i < ld; i++) {
		int c = (unsigned char) ln[i];
		if (c == ' ' || c == '.')
			continue;
		c = ci_upper(c);
		if (!vf_short_char(c))
			c = '_';
		if (nb < 8)
			base[nb++] = (char) c;
	}
	if (ld < n) {
		for (i = ld + 1; i < n; i++) {
			int c = (unsigned char) ln[i];
			if (c == ' ' || c == '.')
				continue;
			c = ci_upper(c);
			if (!vf_short_char(c))
				c = '_';
			if (ne < 3)
				ext[ne++] = (char) c;
		}
	}
	if (nb == 0)
		base[nb++] = '_';
	for (nn = 1; nn <= 999; nn++) {
		unsigned int digits = nn < 10 ? 1 : (nn < 100 ? 2 : 3);
		unsigned int keep = 7 - digits, k = 0, d;
		unsigned int div = digits == 3 ? 100 : (digits == 2 ? 10 : 1);

		for (i = 0; i < 11; i++)
			out[i] = ' ';
		for (i = 0; i < nb && i < keep; i++)
			out[k++] = (unsigned char) base[i];
		out[k++] = '~';
		for (d = 0; d < digits; d++) {
			out[k++] = (unsigned char) ('0' + (nn / div) % 10);
			div /= 10;
			if (div == 0)
				div = 1;
		}
		for (i = 0; i < ne; i++)
			out[8 + i] = (unsigned char) ext[i];
		rc = dir_search(v, dir, out, &r);
		if (rc != FAT_OK)
			return rc;
		if (!r.found)
			return FAT_OK;
	}
	return FAT_ERR_EXIST;
}

/* How the (not yet existing) last component of p will be stored. */
static int vplan_make(struct fat16_vol *v, const struct fat16_path *p, struct vplan *pl)
{
	unsigned int i;
	int rc;

	if (p->short_ok) {
		for (i = 0; i < 11; i++)
			pl->alias[i] = p->name[i];
		pl->nlfn = 0;
		return FAT_OK;
	}
	rc = vf_alias(v, p->dir, p->lname, p->llen, pl->alias);
	if (rc != FAT_OK)
		return rc;
	pl->nlfn = (p->llen + 12u) / 13u;
	return FAT_OK;
}

/* Write the entries of a planned name at the run starting at idx (dir_alloc_run(nlfn + 1) found it): the short
 * entry first, then the long-name entries, ordinal 1 next to the short entry.  A cut between the writes leaves a
 * short entry without its long name, or a chain that is not trusted; never a wrong name. */
static int vcommit(struct fat16_vol *v, unsigned int dir, unsigned long idx, const struct vplan *pl,
                   const char *lname, unsigned int llen, const unsigned char *shortent,
                   unsigned int *olba, unsigned int *ooff)
{
	unsigned char sec[FAT_SECTOR_BYTES], e[32];
	struct dircur cur;
	unsigned int lba = 0, eoff = 0, i, j, k, sum;
	int rc;

	cur.cl = 0;
	cur.k = 0;
	rc = dir_read_entry_sec(v, dir, &cur, idx + pl->nlfn, sec, &lba, &eoff);
	if (rc != FAT_OK)
		return rc == 1 ? FAT_ERR_CHAIN : rc;
	for (i = 0; i < 32; i++)
		sec[eoff + i] = shortent[i];
	rc = wr_sector(v, lba, sec);
	if (rc != FAT_OK)
		return rc;
	if (olba != 0)
		*olba = lba;
	if (ooff != 0)
		*ooff = eoff;
	sum = vf_sum(shortent);
	for (k = 1; k <= pl->nlfn; k++) {
		for (i = 0; i < 32; i++)
			e[i] = 0;
		e[0] = (unsigned char) (k | (k == pl->nlfn ? 0x40u : 0u));
		e[11] = 0x0f;
		e[13] = (unsigned char) sum;
		for (j = 0; j < 13; j++) {
			unsigned int pos = (k - 1u) * 13u + j;
			unsigned int u = pos < llen ? (unsigned char) lname[pos] : (pos == llen ? 0u : 0xffffu);
			e[lfn_off[j]] = (unsigned char) (u & 0xffu);
			e[lfn_off[j] + 1] = (unsigned char) (u >> 8);
		}
		rc = dir_put_entry(v, dir, idx + pl->nlfn - k, e);
		if (rc != FAT_OK)
			return rc;
	}
	return FAT_OK;
}

/* Mark the entry r and its trusted long-name chain deleted: chain first (top entry first), then the short entry. */
static int entry_delete(struct fat16_vol *v, unsigned int dir, const struct dfind *r)
{
	unsigned char sec[FAT_SECTOR_BYTES];
	unsigned int k;
	int rc;

	for (k = r->lfn_n; k >= 1; k--) {
		struct dircur cur;
		unsigned int lba = 0, eoff = 0;
		cur.cl = 0;
		cur.k = 0;
		rc = dir_read_entry_sec(v, dir, &cur, r->idx - k, sec, &lba, &eoff);
		if (rc != FAT_OK)
			return rc == 1 ? FAT_ERR_CHAIN : rc;
		sec[eoff] = 0xe5;
		rc = wr_sector(v, lba, sec);
		if (rc != FAT_OK)
			return rc;
	}
	rc = rd_sector(v, r->lba, sec);
	if (rc != FAT_OK)
		return rc;
	sec[r->off] = 0xe5;
	return wr_sector(v, r->lba, sec);
}
#else
/* Delete the entry r (short names only). */
static int entry_delete(struct fat16_vol *v, unsigned int dir, const struct dfind *r)
{
	unsigned char sec[FAT_SECTOR_BYTES];
	int rc;

	(void) dir;
	rc = rd_sector(v, r->lba, sec);
	if (rc != FAT_OK)
		return rc;
	sec[r->off] = 0xe5;
	return wr_sector(v, r->lba, sec);
}
#endif

#ifdef QDOS_VFAT
#define PFIND(v, p, r) path_find((v), (p)->dir, (p)->name, (p)->short_ok, (p)->lname, (p)->llen, (r))
#else
#define PFIND(v, p, r) dir_search((v), (p)->dir, (p)->name, (r))
#endif

int fat16_resolve(struct fat16_vol *v, unsigned int cwd, const char *path, unsigned int pf,
                  struct fat16_path *out)
{
	unsigned int dir, depth = 0, len = 0, pos = 0, i;
	int rc;

	if (!v->mounted)
		return FAT_ERR_BPB;
	while (path[len] != 0) {
		if (++len > FAT_PATH_MAX)
			return FAT_ERR_DEPTH;
	}
	if (cwd != 0 && (cwd < 2 || cwd > v->clusters + 1))
		return FAT_ERR_CHAIN;
	{                                    /* count the components first: too deep is refused before any lookup */
		unsigned int k = 0, comps = 0;
		while (k < len) {
			while (k < len && path[k] == '/')
				k++;
			if (k < len) {
				comps++;
				while (k < len && path[k] != '/')
					k++;
			}
		}
		if (comps > FAT_PATH_DEPTH)
			return FAT_ERR_DEPTH;
	}
	dir = cwd;
	if (len > 0 && path[0] == '/') {
		dir = 0;
	}
	out->has_name = 0;
	out->trailing_slash = 0;
	for (i = 0; i < 11; i++)
		out->name[i] = ' ';
	out->dir = dir;
#ifdef QDOS_VFAT
	out->short_ok = 0;
	out->llen = 0;
	out->lname[0] = 0;
#endif

	for (;;) {
		unsigned int cs, ce, more;

		while (pos < len && path[pos] == '/')
			pos++;
		if (pos >= len) {
			/* the path is exhausted: it ended in '/', or was empty/only slashes */
			out->dir = dir;
			out->has_name = 0;
			out->trailing_slash = (len > 0 && path[len - 1] == '/');
			return FAT_OK;
		}
		cs = pos;
		while (pos < len && path[pos] != '/')
			pos++;
		ce = pos;
		more = pos;
		while (more < len && path[more] == '/')
			more++;
		if (++depth > FAT_PATH_DEPTH)
			return FAT_ERR_DEPTH;

		if (ce - cs == 1 && path[cs] == '.') {
			if (more >= len) {
				out->dir = dir;
				out->has_name = 0;
				out->trailing_slash = (pos < len);
				return FAT_OK;
			}
			continue;
		}
		if (ce - cs == 2 && path[cs] == '.' && path[cs + 1] == '.') {
			unsigned int up = 0;
			rc = dir_parent(v, dir, &up);
			if (rc != FAT_OK)
				return rc;
			dir = up;
			if (more >= len) {
				out->dir = dir;
				out->has_name = 0;
				out->trailing_slash = (pos < len);
				return FAT_OK;
			}
			continue;
		}
		{
			unsigned char nm[11];
#ifdef QDOS_VFAT
			int sok = 0;
			rc = vf_comp(path + cs, ce - cs, pf, nm, &sok, out->lname);
			out->llen = ce - cs;
			out->short_ok = sok;
#else
			rc = comp83(path + cs, ce - cs, pf, nm);
#endif
			if (rc != FAT_OK)
				return rc;
			if (more >= len) {           /* the last component */
				out->dir = dir;
				for (i = 0; i < 11; i++)
					out->name[i] = nm[i];
				out->has_name = 1;
				out->trailing_slash = (pos < len);
				return FAT_OK;
			}
			{
				struct dfind r;
				unsigned int first;
#ifdef QDOS_VFAT
				rc = path_find(v, dir, nm, sok, out->lname, out->llen, &r);
#else
				rc = dir_search(v, dir, nm, &r);
#endif
				if (rc != FAT_OK)
					return rc;
				if (!r.found)
					return FAT_ERR_NOFILE;
				if (!(r.ent[11] & 0x10))
					return FAT_ERR_NOTDIR;
				first = rd16(r.ent + 26);
				if (first < 2 || first > v->clusters + 1)
					return FAT_ERR_CHAIN;
				dir = first;
			}
		}
	}
}

int fat16_stat_path(struct fat16_vol *v, unsigned int cwd, const char *path, unsigned int pf,
                    struct fat16_info *info)
{
	struct fat16_path p;
	struct dfind r;
	int rc;

	rc = fat16_resolve(v, cwd, path, pf, &p);
	if (rc != FAT_OK)
		return rc;
	if (!p.has_name) {
		info->attr = 0x10;
		info->first = p.dir;
		info->size = 0;
		info->lba = 0;
		info->off = 0xffffu;
		info->dir = p.dir;
		return FAT_OK;
	}
	rc = PFIND(v, &p, &r);
	if (rc != FAT_OK)
		return rc;
	if (!r.found)
		return FAT_ERR_NOFILE;
	info->attr = r.ent[11];
	info->first = rd16(r.ent + 26);
	info->size = rd32(r.ent + 28);
	info->lba = r.lba;
	info->off = r.off;
	info->dir = p.dir;
	if ((info->attr & 0x10) == 0 && p.trailing_slash)
		return FAT_ERR_NOTDIR;
	return FAT_OK;
}

int fat16_chdir_path(struct fat16_vol *v, unsigned int cwd, const char *path, unsigned int pf,
                     unsigned int *newdir)
{
	struct fat16_info in;
	int rc = fat16_stat_path(v, cwd, path, pf, &in);
	if (rc != FAT_OK)
		return rc;
	if (!(in.attr & 0x10))
		return FAT_ERR_NOTDIR;
	if (in.first != 0 && (in.first < 2 || in.first > v->clusters + 1))
		return FAT_ERR_CHAIN;
	*newdir = in.first;
	return FAT_OK;
}

/* ---- listing --------------------------------------------------------------------------- */

int fat16_dir_list_open(struct fat16_vol *v, unsigned int dir, unsigned int lflags, struct fat16_file *f)
{
	if (!v->mounted)
		return FAT_ERR_BPB;
	if (dir != 0 && (dir < 2 || dir > v->clusters + 1))
		return FAT_ERR_CHAIN;
	f->vol = v;
	f->first = dir;
	f->size = 0;
	f->pos = 0;
	f->cur = 0;
	f->cur_idx = 0;
	f->dir_lba = 0;
	f->dir_off = 0;
	f->is_dir = 1;
	f->dir_next = 0;
	f->mode = FAT_O_READ;
	f->dirty = 0;
	f->lflags = lflags;
	return FAT_OK;
}

#ifdef QDOS_VFAT
/* VFAT build: as below, plus the long name (out->lname) and the attribute.  The alias is always in out->name. */
int fat16_dir_list_nextl(struct fat16_file *f, struct fat16_ldirent *out)
{
	struct fat16_vol *v = f->vol;
	unsigned char sec[FAT_SECTOR_BYTES], ent[32];
	struct dircur cur;
	struct lfnc lc;

	lc.active = 0;
	cur.cl = 0;
	cur.k = 0;
	while (f->dir_next < DIR_MAX_ENTRIES) {
		unsigned long idx = f->dir_next;
		unsigned char *p;
		unsigned int lba = 0, eoff = 0, blen = 8, elen = 3, i, k;
		int rc, isdir, chain;

		rc = dir_read_entry_sec(v, f->first, &cur, idx, sec, &lba, &eoff);
		if (rc == 1) {
			f->dir_next = (unsigned int) DIR_MAX_ENTRIES;
			return 0;
		}
		if (rc != FAT_OK)
			return rc;
		p = sec + eoff;
		f->dir_next++;
		if (p[0] == 0x00) {
			f->dir_next = (unsigned int) DIR_MAX_ENTRIES;
			return 0;
		}
		if (p[0] == 0xe5) {
			lc.active = 0;
			continue;
		}
		if (p[11] == 0x0f) {
			lfnc_feed(&lc, p);
			continue;
		}
		chain = lfnc_ok(&lc, p);
		lc.active = 0;
		isdir = (p[11] & 0x10) != 0;
		if ((p[11] & 0x08) && !isdir)
			continue;                    /* volume label */
		if (isdir && (!(f->lflags & FATL_DIRS) || p[0] == '.'))
			continue;
		for (i = 0; i < 32; i++)
			ent[i] = p[i];
		while (blen > 0 && ent[blen - 1] == ' ')
			blen--;
		while (elen > 0 && ent[8 + elen - 1] == ' ')
			elen--;
		if (blen == 0) {
			fat16_dir_skipped++;
			continue;
		}
		for (i = 0; i < blen; i++)
			if (!name_byte_ok(ent[i]))
				break;
		if (i != blen) {
			fat16_dir_skipped++;
			continue;
		}
		for (i = 0; i < elen; i++)
			if (!name_byte_ok(ent[8 + i]))
				break;
		if (i != elen) {
			fat16_dir_skipped++;
			continue;
		}
		k = 0;
		for (i = 0; i < blen; i++)
			out->name[k++] = (char) ent[i];
		if (elen != 0) {
			out->name[k++] = '.';
			for (i = 0; i < elen; i++)
				out->name[k++] = (char) ent[8 + i];
		}
		while (k < 12)
			out->name[k++] = 0;
		out->size = isdir ? 0xffffffffUL : rd32(ent + 28);
		out->attr = ent[11];
		out->has_lfn = 0;
		if (chain && lfnc_name(&lc)) {
			for (i = 0; i <= lc.len; i++)
				out->lname[i] = lc.s[i];
			out->has_lfn = 1;
		} else {
			for (i = 0; i < 12; i++)
				out->lname[i] = out->name[i];
			out->lname[12] = 0;
			if (chain)
				out->has_lfn = 2;
		}
		return 1;
	}
	return 0;
}

int fat16_dir_list_next(struct fat16_file *f, struct fat16_dirent *out)
{
	struct fat16_ldirent l;
	unsigned int i;
	int rc = fat16_dir_list_nextl(f, &l);

	if (rc == 1) {
		for (i = 0; i < 12; i++)
			out->name[i] = l.name[i];
		out->size = l.size;
	}
	return rc;
}
#else
int fat16_dir_list_next(struct fat16_file *f, struct fat16_dirent *out)
{
	struct fat16_vol *v = f->vol;
	unsigned char sec[FAT_SECTOR_BYTES];
	struct dircur cur;

	cur.cl = 0;
	cur.k = 0;
	while (f->dir_next < DIR_MAX_ENTRIES) {
		unsigned long idx = f->dir_next;
		unsigned char *p;
		unsigned int lba = 0, eoff = 0, blen = 8, elen = 3, i, k;
		int rc, isdir;

		rc = dir_read_entry_sec(v, f->first, &cur, idx, sec, &lba, &eoff);
		if (rc == 1) {
			f->dir_next = (unsigned int) DIR_MAX_ENTRIES;
			return 0;
		}
		if (rc != FAT_OK)
			return rc;
		p = sec + eoff;
		f->dir_next++;
		if (p[0] == 0x00) {
			f->dir_next = (unsigned int) DIR_MAX_ENTRIES;
			return 0;
		}
		if (p[0] == 0xe5 || p[11] == 0x0f)
			continue;
		isdir = (p[11] & 0x10) != 0;
		if ((p[11] & 0x08) && !isdir)
			continue;                    /* volume label */
		if (isdir && (!(f->lflags & FATL_DIRS) || p[0] == '.'))
			continue;
		while (blen > 0 && p[blen - 1] == ' ')
			blen--;
		while (elen > 0 && p[8 + elen - 1] == ' ')
			elen--;
		if (blen == 0) {
			fat16_dir_skipped++;
			continue;
		}
		for (i = 0; i < blen; i++)
			if (!name_byte_ok(p[i]))
				break;
		if (i != blen) {
			fat16_dir_skipped++;
			continue;
		}
		for (i = 0; i < elen; i++)
			if (!name_byte_ok(p[8 + i]))
				break;
		if (i != elen) {
			fat16_dir_skipped++;
			continue;
		}
		k = 0;
		for (i = 0; i < blen; i++)
			out->name[k++] = (char) p[i];
		if (elen != 0) {
			out->name[k++] = '.';
			for (i = 0; i < elen; i++)
				out->name[k++] = (char) p[8 + i];
		}
		while (k < 12)
			out->name[k++] = 0;
		out->size = isdir ? 0xffffffffUL : rd32(p + 28);
		return 1;
	}
	return 0;
}
#endif

/* ---- open / create ----------------------------------------------------------------------- */

static void ent_new(unsigned char *e, const unsigned char *name, unsigned int attr, unsigned int first)
{
	unsigned int i;
	for (i = 0; i < 32; i++)
		e[i] = 0;
	for (i = 0; i < 11; i++)
		e[i] = name[i];
	e[11] = (unsigned char) attr;
	stamp(e);
	wr16(e + 26, first);
}

int fat16_open_path(struct fat16_vol *v, unsigned int cwd, const char *path, unsigned int pf,
                    unsigned int flags, struct fat16_file *f)
{
	struct fat16_path p;
	struct dfind r;
	int rc;

	if (!v->mounted)
		return FAT_ERR_BPB;
	if (!(flags & (FAT_O_READ | FAT_O_WRITE)))
		return FAT_ERR_WPROT;
	rc = fat16_resolve(v, cwd, path, pf, &p);
	if (rc != FAT_OK)
		return rc;
	if (!p.has_name) {                   /* the directory itself */
		if (flags & FAT_O_WRITE)
			return FAT_ERR_ISDIR;
		return fat16_dir_list_open(v, p.dir, 0, f);
	}
	if (flags & FAT_O_WRITE) {
		rc = probe_write(v);
		if (rc != FAT_OK)
			return rc;
	}
	rc = PFIND(v, &p, &r);
	if (rc != FAT_OK)
		return rc;

	f->vol = v;
	f->is_dir = 0;
	f->dir_next = 0;
	f->pos = 0;
	f->cur_idx = 0;
	f->mode = (flags & FAT_O_WRITE) ? flags : FAT_O_READ;
	f->dirty = 0;
	f->lflags = 0;

	if (r.found) {
		unsigned int first = rd16(r.ent + 26);
		if (r.ent[11] & 0x10) {          /* a directory */
			if (flags & FAT_O_WRITE)
				return FAT_ERR_ISDIR;
			if (first < 2 || first > v->clusters + 1)
				return FAT_ERR_CHAIN;
			return fat16_dir_list_open(v, first, 0, f);
		}
		if (p.trailing_slash)
			return FAT_ERR_NOTDIR;
		if ((flags & FAT_O_WRITE) && (flags & FAT_O_CREATE) && (flags & FAT_O_EXCL))
			return FAT_ERR_EXIST;
		f->first = first;
		f->size = rd32(r.ent + 28);
		f->cur = f->first;
		f->dir_lba = r.lba;
		f->dir_off = r.off;
		if (f->size != 0 && (f->first < 2 || f->first > v->clusters + 1))
			return FAT_ERR_CHAIN;
		rc = chain_check(v, f->first, f->size);
		if (rc != FAT_OK)
			return rc;
		if ((flags & FAT_O_WRITE) && (flags & FAT_O_TRUNC))
			return fat16_truncate(f, 0);
		return FAT_OK;
	}
	if (!(flags & FAT_O_WRITE) || !(flags & FAT_O_CREATE))
		return FAT_ERR_NOFILE;
	if (p.trailing_slash)
		return FAT_ERR_NOTDIR;
#ifdef QDOS_VFAT
	{
		unsigned char ent[32];
		struct vplan pl;
		unsigned long idx = 0;
		unsigned int lba = 0, eoff = 0;

		rc = vplan_make(v, &p, &pl);
		if (rc != FAT_OK)
			return rc;
		rc = dir_alloc_run(v, p.dir, pl.nlfn + 1, &idx);
		if (rc != FAT_OK)
			return rc;
		ent_new(ent, pl.alias, 0x20, 0);
		rc = vcommit(v, p.dir, idx, &pl, p.lname, p.llen, ent, &lba, &eoff);
		if (rc != FAT_OK)
			return rc;
		f->first = 0;
		f->size = 0;
		f->cur = 0;
		f->dir_lba = lba;
		f->dir_off = eoff;
	}
	return FAT_OK;
#else
	{
		unsigned char ent[32], sec[FAT_SECTOR_BYTES];
		struct dircur cur;
		unsigned long idx = 0;
		unsigned int lba = 0, eoff = 0, i;

		rc = dir_alloc_run(v, p.dir, 1, &idx);
		if (rc != FAT_OK)
			return rc;
		ent_new(ent, p.name, 0x20, 0);
		cur.cl = 0;
		cur.k = 0;
		rc = dir_read_entry_sec(v, p.dir, &cur, idx, sec, &lba, &eoff);
		if (rc != FAT_OK)
			return rc == 1 ? FAT_ERR_CHAIN : rc;
		for (i = 0; i < 32; i++)
			sec[eoff + i] = ent[i];
		rc = wr_sector(v, lba, sec);
		if (rc != FAT_OK)
			return rc;
		f->first = 0;
		f->size = 0;
		f->cur = 0;
		f->dir_lba = lba;
		f->dir_off = eoff;
	}
	return FAT_OK;
#endif
}

/* ---- mkdir / rmdir / remove / rename ---------------------------------------------------- */

int fat16_mkdir(struct fat16_vol *v, unsigned int cwd, const char *path, unsigned int pf)
{
	struct fat16_path p;
	struct dfind r;
	unsigned char sec[FAT_SECTOR_BYTES], ent[32];
	unsigned long idx = 0;
	unsigned int nc, i, parent;
	int rc;
#ifdef QDOS_VFAT
	struct vplan pl;
#endif

	if (!v->mounted)
		return FAT_ERR_BPB;
	rc = probe_write(v);
	if (rc != FAT_OK)
		return rc;
	rc = fat16_resolve(v, cwd, path, pf, &p);
	if (rc != FAT_OK)
		return rc;
	if (!p.has_name)
		return FAT_ERR_EXIST;            /* "/", ".", ".." name existing directories */
	rc = PFIND(v, &p, &r);
	if (rc != FAT_OK)
		return rc;
	if (r.found)
		return FAT_ERR_EXIST;
	parent = p.dir;
#ifdef QDOS_VFAT
	rc = vplan_make(v, &p, &pl);
	if (rc != FAT_OK)
		return rc;
	rc = dir_alloc_run(v, parent, pl.nlfn + 1, &idx);   /* may grow the parent; fails before any cluster is taken */
#else
	rc = dir_alloc_run(v, parent, 1, &idx);       /* may grow the parent; fails before any cluster is taken */
#endif
	if (rc != FAT_OK)
		return rc;
	rc = fat_find_free(v, &nc);
	if (rc != FAT_OK)
		return rc;
	/* first sector: "." and ".."; every other sector of the cluster zero */
	for (i = 0; i < FAT_SECTOR_BYTES; i++)
		sec[i] = 0;
	{
		unsigned char dot[11], dotdot[11];
		for (i = 0; i < 11; i++) {
			dot[i] = ' ';
			dotdot[i] = ' ';
		}
		dot[0] = '.';
		dotdot[0] = '.';
		dotdot[1] = '.';
		ent_new(ent, dot, 0x10, nc);
		for (i = 0; i < 32; i++)
			sec[i] = ent[i];
		ent_new(ent, dotdot, 0x10, parent);
		for (i = 0; i < 32; i++)
			sec[32 + i] = ent[i];
	}
	rc = cluster_zero(v, nc);
	if (rc != FAT_OK)
		return rc;
	rc = wr_sector(v, v->data_start + (unsigned int) (nc - 2u) * v->spc, sec);
	if (rc != FAT_OK)
		return rc;
	rc = fat_set(v, nc, 0xffff);
	if (rc != FAT_OK)
		return rc;
#ifdef QDOS_VFAT
	ent_new(ent, pl.alias, 0x10, nc);
	return vcommit(v, parent, idx, &pl, p.lname, p.llen, ent, 0, 0);
#else
	ent_new(ent, p.name, 0x10, nc);
	return dir_put_entry(v, parent, idx, ent);
#endif
}

/* Is directory dir empty (only "." and ".." and deleted entries)? */
static int dir_is_empty(struct fat16_vol *v, unsigned int dir, int *empty)
{
	unsigned char sec[FAT_SECTOR_BYTES];
	struct dircur cur;
	unsigned long idx = 0;
	unsigned int lba = 0, eoff = 0;
	int rc;

	cur.cl = 0;
	cur.k = 0;
	*empty = 1;
	while (idx < DIR_MAX_ENTRIES) {
		unsigned char *p;
		rc = dir_read_entry_sec(v, dir, &cur, idx, sec, &lba, &eoff);
		if (rc == 1)
			return FAT_OK;
		if (rc != FAT_OK)
			return rc;
		p = sec + eoff;
		if (p[0] == 0x00)
			return FAT_OK;
#ifdef QDOS_VFAT
		if (p[0] != 0xe5 && p[11] != 0x0f) {         /* long-name entries alone do not make a directory non-empty */
#else
		if (p[0] != 0xe5) {
#endif
			if (!(idx < 2 && p[0] == '.' && (p[11] & 0x10)))
				*empty = 0;
			if (!*empty)
				return FAT_OK;
		}
		idx++;
	}
	return FAT_OK;
}

int fat16_rmdir(struct fat16_vol *v, unsigned int cwd, const char *path, unsigned int pf)
{
	struct fat16_path p;
	struct dfind r;
	unsigned int first;
	int rc, empty;

	if (!v->mounted)
		return FAT_ERR_BPB;
	rc = probe_write(v);
	if (rc != FAT_OK)
		return rc;
	rc = fat16_resolve(v, cwd, path, pf, &p);
	if (rc != FAT_OK)
		return rc;
	if (!p.has_name)
		return FAT_ERR_INVAL;
	rc = PFIND(v, &p, &r);
	if (rc != FAT_OK)
		return rc;
	if (!r.found)
		return FAT_ERR_NOFILE;
	if (!(r.ent[11] & 0x10))
		return FAT_ERR_NOTDIR;
	first = rd16(r.ent + 26);
	if (first < 2 || first > v->clusters + 1)
		return FAT_ERR_CHAIN;
	rc = dir_is_empty(v, first, &empty);
	if (rc != FAT_OK)
		return rc;
	if (!empty)
		return FAT_ERR_NOTEMPTY;
	rc = entry_delete(v, p.dir, &r);
	if (rc != FAT_OK)
		return rc;
	return chain_free(v, first);
}

int fat16_remove_path(struct fat16_vol *v, unsigned int cwd, const char *path, unsigned int pf)
{
	struct fat16_path p;
	struct dfind r;
	unsigned int first;
	unsigned long size;
	int rc;

	if (!v->mounted)
		return FAT_ERR_BPB;
	rc = probe_write(v);
	if (rc != FAT_OK)
		return rc;
	rc = fat16_resolve(v, cwd, path, pf, &p);
	if (rc != FAT_OK)
		return rc;
	if (!p.has_name)
		return FAT_ERR_ISDIR;
	rc = PFIND(v, &p, &r);
	if (rc != FAT_OK)
		return rc;
	if (!r.found)
		return FAT_ERR_NOFILE;
	if (r.ent[11] & 0x10)
		return FAT_ERR_ISDIR;
	if (p.trailing_slash)
		return FAT_ERR_NOTDIR;
	first = rd16(r.ent + 26);
	size = rd32(r.ent + 28);
	if (size != 0 && (first < 2 || first > v->clusters + 1))
		return FAT_ERR_CHAIN;
	rc = chain_check(v, first, size);
	if (rc != FAT_OK)
		return rc;
	rc = entry_delete(v, p.dir, &r);
	if (rc != FAT_OK)
		return rc;
	if (size == 0)
		return FAT_OK;
	return chain_free(v, first);
}

#ifdef QDOS_VFAT
int fat16_rename_path(struct fat16_vol *v, unsigned int cwd, const char *from, const char *to,
                      unsigned int pf)
{
	struct fat16_path pf_, pt;
	struct dfind rf, rt;
	struct vplan pl;
	unsigned char sec[FAT_SECTOR_BYTES], ent[32];
	unsigned long idx = 0;
	unsigned int i;
	int rc;

	if (!v->mounted)
		return FAT_ERR_BPB;
	rc = probe_write(v);
	if (rc != FAT_OK)
		return rc;
	rc = fat16_resolve(v, cwd, from, pf, &pf_);
	if (rc != FAT_OK)
		return rc;
	rc = fat16_resolve(v, cwd, to, pf, &pt);
	if (rc != FAT_OK)
		return rc;
	if (!pf_.has_name || !pt.has_name)
		return FAT_ERR_INVAL;
	rc = PFIND(v, &pf_, &rf);
	if (rc != FAT_OK)
		return rc;
	if (!rf.found)
		return FAT_ERR_NOFILE;
	rc = PFIND(v, &pt, &rt);
	if (rc != FAT_OK)
		return rc;
	if (rt.found) {
		if (rt.lba == rf.lba && rt.off == rf.off)
			return FAT_OK;               /* the same entry */
		return FAT_ERR_EXIST;
	}
	if ((rf.ent[11] & 0x10) && pf_.dir != pt.dir)
		return FAT_ERR_XDIR;
	if (pf_.dir == pt.dir && rf.lfn_n == 0 && pt.short_ok) {   /* same directory, short to short: rewrite the name */
		rc = rd_sector(v, rf.lba, sec);
		if (rc != FAT_OK)
			return rc;
		for (i = 0; i < 11; i++)
			sec[rf.off + i] = pt.name[i];
		return wr_sector(v, rf.lba, sec);
	}
	/* new entries first (short entry, then its long name), then delete the old ones: a cut leaves two names for
	 * one chain, which the checker reports; nothing is lost */
	rc = vplan_make(v, &pt, &pl);
	if (rc != FAT_OK)
		return rc;
	rc = dir_alloc_run(v, pt.dir, pl.nlfn + 1, &idx);
	if (rc != FAT_OK)
		return rc;
	for (i = 0; i < 32; i++)
		ent[i] = rf.ent[i];
	for (i = 0; i < 11; i++)
		ent[i] = pl.alias[i];
	rc = vcommit(v, pt.dir, idx, &pl, pt.lname, pt.llen, ent, 0, 0);
	if (rc != FAT_OK)
		return rc;
	return entry_delete(v, pf_.dir, &rf);
}
#else
int fat16_rename_path(struct fat16_vol *v, unsigned int cwd, const char *from, const char *to,
                      unsigned int pf)
{
	struct fat16_path pf_, pt;
	struct dfind rf, rt;
	unsigned char sec[FAT_SECTOR_BYTES], ent[32];
	unsigned long idx = 0;
	unsigned int i;
	int rc;

	if (!v->mounted)
		return FAT_ERR_BPB;
	rc = probe_write(v);
	if (rc != FAT_OK)
		return rc;
	rc = fat16_resolve(v, cwd, from, pf, &pf_);
	if (rc != FAT_OK)
		return rc;
	rc = fat16_resolve(v, cwd, to, pf, &pt);
	if (rc != FAT_OK)
		return rc;
	if (!pf_.has_name || !pt.has_name)
		return FAT_ERR_INVAL;
	rc = PFIND(v, &pf_, &rf);
	if (rc != FAT_OK)
		return rc;
	if (!rf.found)
		return FAT_ERR_NOFILE;
	rc = PFIND(v, &pt, &rt);
	if (rc != FAT_OK)
		return rc;
	if (rt.found) {
		if (rt.lba == rf.lba && rt.off == rf.off)
			return FAT_OK;               /* the same entry */
		return FAT_ERR_EXIST;
	}
	if (pf_.dir == pt.dir) {             /* same directory: rewrite the name */
		rc = rd_sector(v, rf.lba, sec);
		if (rc != FAT_OK)
			return rc;
		for (i = 0; i < 11; i++)
			sec[rf.off + i] = pt.name[i];
		return wr_sector(v, rf.lba, sec);
	}
	if (rf.ent[11] & 0x10)
		return FAT_ERR_XDIR;
	/* move a file: new entry first, then delete the old one (a cut leaves two names for one chain,
	 * which the checker reports; nothing is lost) */
	rc = dir_alloc_run(v, pt.dir, 1, &idx);
	if (rc != FAT_OK)
		return rc;
	for (i = 0; i < 32; i++)
		ent[i] = rf.ent[i];
	for (i = 0; i < 11; i++)
		ent[i] = pt.name[i];
	rc = dir_put_entry(v, pt.dir, idx, ent);
	if (rc != FAT_OK)
		return rc;
	rc = rd_sector(v, rf.lba, sec);
	if (rc != FAT_OK)
		return rc;
	sec[rf.off] = 0xe5;
	return wr_sector(v, rf.lba, sec);
}
#endif

/* ---- getcwd ------------------------------------------------------------------------------ */

/* Name of the entry in directory dir whose first cluster is child; out gets "NAME.EXT\0". */
static int dir_name_of(struct fat16_vol *v, unsigned int dir, unsigned int child, char *out)
{
	unsigned char sec[FAT_SECTOR_BYTES];
	struct dircur cur;
	unsigned long idx;
	unsigned int lba = 0, eoff = 0, i, k, blen, elen;
	int rc;
#ifdef QDOS_VFAT
	struct lfnc lc;
	int chain = 0;

	lc.active = 0;
#endif

	cur.cl = 0;
	cur.k = 0;
	for (idx = 0; idx < DIR_MAX_ENTRIES; idx++) {
		unsigned char *p;
		rc = dir_read_entry_sec(v, dir, &cur, idx, sec, &lba, &eoff);
		if (rc == 1)
			return FAT_ERR_CHAIN;
		if (rc != FAT_OK)
			return rc;
		p = sec + eoff;
		if (p[0] == 0x00)
			return FAT_ERR_CHAIN;
#ifdef QDOS_VFAT
		if (p[0] == 0xe5) {
			lc.active = 0;
			continue;
		}
		if (p[11] == 0x0f) {
			lfnc_feed(&lc, p);
			continue;
		}
		chain = lfnc_ok(&lc, p);
		lc.active = 0;
#endif
		if (p[0] == 0xe5 || p[11] == 0x0f || !(p[11] & 0x10) || p[0] == '.')
			continue;
		if (rd16(p + 26) != child)
			continue;
		blen = 8;
		elen = 3;
		while (blen > 0 && p[blen - 1] == ' ')
			blen--;
		while (elen > 0 && p[8 + elen - 1] == ' ')
			elen--;
		k = 0;
		for (i = 0; i < blen; i++)
			out[k++] = (char) p[i];
		if (elen != 0) {
			out[k++] = '.';
			for (i = 0; i < elen; i++)
				out[k++] = (char) p[8 + i];
		}
		out[k] = 0;
#ifdef QDOS_VFAT
		if (chain && lfnc_name(&lc)) {   /* the long name, if the entry has a trusted representable one */
			for (i = 0; i <= lc.len; i++)
				out[i] = lc.s[i];
		}
#endif
		return FAT_OK;
	}
	return FAT_ERR_CHAIN;
}

#ifdef QDOS_VFAT
/* VFAT build: components are long names where the entry has one; the path is built from the leaf upward. */
int fat16_getcwd(struct fat16_vol *v, unsigned int dir, char *buf, unsigned int n)
{
	char tmp[264], nm[FAT_LFN_MAX + 1];
	unsigned int depth = 0, pos = sizeof tmp - 1, cur = dir, l, i, total;
	int rc;

	if (!v->mounted)
		return FAT_ERR_BPB;
	if (n < 2)
		return FAT_ERR_DEPTH;
	tmp[pos] = 0;
	while (cur != 0) {
		unsigned int up = 0;
		if (depth >= FAT_PATH_DEPTH)
			return FAT_ERR_DEPTH;
		rc = dir_parent(v, cur, &up);
		if (rc != FAT_OK)
			return rc;
		rc = dir_name_of(v, up, cur, nm);
		if (rc != FAT_OK)
			return rc;
		for (l = 0; nm[l] != 0; l++)
			;
		if (l + 1 > pos)
			return FAT_ERR_DEPTH;
		pos -= l;
		for (i = 0; i < l; i++)
			tmp[pos + i] = nm[i];
		tmp[--pos] = '/';
		depth++;
		cur = up;
	}
	if (depth == 0) {
		buf[0] = '/';
		buf[1] = 0;
		return 1;
	}
	total = sizeof tmp - 1 - pos;
	if (total + 1 > n)
		return FAT_ERR_DEPTH;
	for (i = 0; i <= total; i++)
		buf[i] = tmp[pos + i];
	return (int) total;
}
#else
int fat16_getcwd(struct fat16_vol *v, unsigned int dir, char *buf, unsigned int n)
{
	char names[FAT_PATH_DEPTH][13];
	unsigned int depth = 0, pos = 0, i, j;
	unsigned int cur = dir;
	int rc;

	if (!v->mounted)
		return FAT_ERR_BPB;
	if (n < 2)
		return FAT_ERR_DEPTH;
	while (cur != 0) {
		unsigned int up = 0;
		if (depth >= FAT_PATH_DEPTH)
			return FAT_ERR_DEPTH;
		rc = dir_parent(v, cur, &up);
		if (rc != FAT_OK)
			return rc;
		rc = dir_name_of(v, up, cur, names[depth]);
		if (rc != FAT_OK)
			return rc;
		depth++;
		cur = up;
	}
	if (depth == 0) {
		buf[0] = '/';
		buf[1] = 0;
		return 1;
	}
	for (i = depth; i > 0; i--) {
		if (pos + 1 >= n)
			return FAT_ERR_DEPTH;
		buf[pos++] = '/';
		for (j = 0; names[i - 1][j] != 0; j++) {
			if (pos + 1 >= n)
				return FAT_ERR_DEPTH;
			buf[pos++] = names[i - 1][j];
		}
	}
	buf[pos] = 0;
	return (int) pos;
}
#endif

#endif /* QDOS_FS_WRITE */
