/*
 * QDOS/386 R6 - shared FAT16 reader.
 *
 * One implementation serves every platform.  The sector source is a callback
 * (fat_read_fn): the Tiny386 IDE backend, the i386VM RAM disk, and the test
 * wrappers that substitute single sectors all use this interface.
 *
 * Scope (see docs/R6-FAT16-FORMAT.md):
 *   - read only, root directory only, 8.3 names in uppercase
 *   - 512-byte sectors, power-of-two sectors per cluster, two FAT copies
 *   - FAT16 only: the cluster count classifies the volume (4085..65524)
 *   - every file's cluster chain is validated at open: entries in range,
 *     no cycles, chain length equal to the file size in clusters
 */
#ifndef QDOS386_FAT16_H
#define QDOS386_FAT16_H

#define FAT_SECTOR_BYTES 512u

/* Errors, negative.  Returned directly to channel users (see win1.c). */
#define FAT_OK          0
#define FAT_ERR_IO    (-40)   /* sector read failed */
#define FAT_ERR_BPB   (-41)   /* boot sector or BPB fields invalid */
#define FAT_ERR_TYPE  (-42)   /* not a FAT16 volume (cluster count) */
#define FAT_ERR_CHAIN (-43)   /* cluster number, cycle or unexpected end */
#define FAT_ERR_NOFILE (-44)  /* no such root-directory entry */
#define FAT_ERR_SIZE  (-45)   /* chain shorter than the file size */
#define FAT_ERR_NAME  (-46)   /* name is not a valid uppercase 8.3 name */
#define FAT_ERR_RANGE (-47)   /* sector or seek position outside the volume */

/* Sector source: fills buf (512 bytes) with sector lba.  Returns 0 on success. */
typedef int (*fat_read_fn)(void *ctx, unsigned int lba, unsigned char *buf);

struct fat16_vol {
	fat_read_fn read;
	void *ctx;
	unsigned int total_sectors;      /* volume size, validated <= device */
	unsigned int fat_start;          /* first FAT sector */
	unsigned int fat_sectors;        /* sectors per FAT copy */
	unsigned int root_start;         /* first root-directory sector */
	unsigned int root_sectors;
	unsigned int data_start;         /* sector of cluster 2 */
	unsigned int spc;                /* sectors per cluster */
	unsigned int clusters;           /* data clusters: valid numbers 2..clusters+1 */
	int mounted;
};

struct fat16_file {
	struct fat16_vol *vol;
	unsigned int first;              /* first cluster (0 for an empty file) */
	unsigned long size;              /* bytes, from the directory entry */
	unsigned long pos;               /* current byte position */
	unsigned int cur;                /* cluster at index cur_idx (read cursor) */
	unsigned long cur_idx;
	unsigned long dir_lba;           /* where the directory entry lives (tests) */
	unsigned int dir_off;
	unsigned int is_dir;             /* R7: an enumeration cursor, not a file */
	unsigned int dir_next;           /* R7: next root entry index to examine */
};

/* One listed root-directory file (R7).  name is "BASE.EXT" in 12 bytes,
 * zero-padded; a full 12-character name has no terminator.  The record that
 * channels return is these 16 bytes: name[12], then size as little-endian u32. */
struct fat16_dirent {
	char name[12];
	unsigned long size;
};

/* Validate the boot sector and FAT16 classification.  device_sectors is the
 * size of the sector source; the volume may not extend past it. */
int fat16_mount(struct fat16_vol *v, fat_read_fn read, void *ctx,
                unsigned long device_sectors);

/* Convert "NAME.EXT" to the 11-byte directory form.  Uppercase A-Z, 0-9 and
 * '_' only; base 1..8 characters, extension 0..3, one dot at most. */
int fat16_name83(const char *name, unsigned char out[11]);

/* Find a root-directory file and validate its cluster chain.  On success f is
 * ready for fat16_read (cursor at the start). */
int fat16_open_root(struct fat16_vol *v, const char *name, struct fat16_file *f);

/* Read up to n bytes at f->pos into dst, advancing pos.  *got receives the
 * count (0 at end of file). */
int fat16_read(struct fat16_file *f, unsigned char *dst, unsigned int n, unsigned int *got);

/* Set the position; positions past the end are refused. */
int fat16_seek(struct fat16_file *f, unsigned long pos);

/* Root-directory enumeration (R7).  fat16_dir_open starts a cursor at the
 * first entry.  fat16_dir_next returns 1 with the next regular file in
 * directory order, 0 at the end, or a negative error.  The end is the first
 * entry whose first byte is 0x00, and it is sticky.  Skipped without a
 * count: deleted (0xE5), volume labels, long-name entries and subdirectories.
 * Counted in fat16_dir_skipped: a regular entry whose 8.3 name is not valid
 * (name bytes outside A-Z 0-9 _, or a space inside the name).  The listing
 * never checks cluster chains; fat16_open_root does. */
int fat16_dir_open(struct fat16_vol *v, struct fat16_file *f);
int fat16_dir_next(struct fat16_file *f, struct fat16_dirent *out);
extern volatile unsigned long fat16_dir_skipped;

#endif
