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

#endif /* QDOS_FS_WRITE */
