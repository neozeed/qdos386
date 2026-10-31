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

#ifdef QDOS_FS_WRITE
/* R11L write errors (opt-in, -DQDOS_FS_WRITE; see docs/R11L-FAT-WRITE-ABI.md). */
#define FAT_ERR_WPROT   (-80)  /* device is write-protected, or the open mode does not allow writing */
#define FAT_ERR_FULL    (-81)  /* no free cluster */
#define FAT_ERR_DIRFULL (-82)  /* root directory has no free entry */
#define FAT_ERR_EXIST   (-83)  /* create-exclusive, rename target, or a directory/label with that name */
#define FAT_ERR_BUSY    (-84)  /* the file is open on another channel (checked by win1.c) */
#define FAT_ERR_WIO     (-85)  /* sector write failed */
#define FAT_ERR_NOTOPEN (-86)  /* operation needs a file opened for it */
/* R11M directory errors (see docs/R11M-FAT-PATH-ABI.md). */
#define FAT_ERR_NOTDIR   (-87)  /* a path component (or the target) is not a directory */
#define FAT_ERR_ISDIR    (-88)  /* the target is a directory */
#define FAT_ERR_NOTEMPTY (-89)  /* rmdir of a directory that holds entries */
#define FAT_ERR_DEPTH    (-90)  /* path too long or nested too deeply */
#define FAT_ERR_INVAL    (-91)  /* not allowed on this target ('.', '..', the root, the current directory) */
#define FAT_ERR_XDIR     (-92)  /* a directory cannot be moved to another parent */

/* Open flags for fat16_open_ex. */
#define FAT_O_READ   0x01u
#define FAT_O_WRITE  0x02u
#define FAT_O_CREATE 0x04u     /* create if missing (needs FAT_O_WRITE) */
#define FAT_O_TRUNC  0x08u     /* truncate to 0 if present (needs FAT_O_WRITE) */
#define FAT_O_APPEND 0x10u     /* every write goes to the end (needs FAT_O_WRITE) */
#define FAT_O_EXCL   0x20u     /* with CREATE: fail if the file exists */

/* Sector sink: writes buf (512 bytes) to sector lba.  0 on success, -2 if write-protected,
 * other negative on error. */
typedef int (*fat_write_fn)(void *ctx, unsigned int lba, const unsigned char *buf);
#endif

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
#ifdef QDOS_FS_WRITE
	fat_write_fn write;              /* 0: read-only volume */
	unsigned int next_free;          /* allocation hint (cluster number) */
	unsigned long sectors_written;   /* diagnostics */
	int wstate;                      /* 0 not probed, 1 writable, -1 write-protected (latched) */
#endif
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
#ifdef QDOS_FS_WRITE
	unsigned int mode;               /* FAT_O_* flags the file was opened with */
	unsigned int dirty;              /* size or first cluster changed, directory entry not yet written */
	unsigned int lflags;             /* R11M listing cursor: FATL_* (is_dir; first = directory cluster, 0 = root) */
#endif
};

/* One listed root-directory file (R7).  name is "BASE.EXT" in 12 bytes,
 * zero-padded; a full 12-character name has no terminator.  The record that
 * channels return is these 16 bytes: name[12], then size as little-endian u32. */
struct fat16_dirent {
	char name[12];
	unsigned long size;
};

#ifdef QDOS_VFAT
/* R11M VFAT: a listing entry with its long name.  name/size as above (name is the 8.3 alias). */
struct fat16_ldirent {
	char name[12];
	unsigned long size;
	char lname[64];              /* the long name (ASCII) if the entry has one, else the 8.3 name; NUL-terminated */
	unsigned int attr;           /* attribute byte */
	unsigned int has_lfn;        /* 1 if lname is a long name, 2 if the entry has a long name that cannot be shown (alias only) */
};
int fat16_dir_list_nextl(struct fat16_file *f, struct fat16_ldirent *out);
#endif

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


#ifdef QDOS_FS_WRITE
/* ---- R11L: writable volume ---------------------------------------------------------------------
 * Every write is write-through.  Order for a growing file: data sector, then FAT (new cluster =
 * end-of-chain, then the predecessor's link; first copy, then second copy), then the directory
 * entry.  The directory entry is also written at fat16_flush / close.  No journal, no power-loss
 * atomicity: a cut between the FAT link and the directory update leaves a chain longer than the
 * size, which fat16_open_ex refuses (FAT_ERR_CHAIN) rather than misreads; the host checker
 * (tools/r11l-fatcheck.py) reports and repairs it. */
void fat16_set_writer(struct fat16_vol *v, fat_write_fn w);

/* Open by flags.  FAT_O_READ alone behaves like fat16_open_root.  A missing file is created with
 * FAT_O_CREATE (FAT_ERR_DIRFULL if the root has no free entry).  FAT_O_TRUNC empties an existing
 * file. */
int fat16_open_ex(struct fat16_vol *v, const char *name, unsigned int flags, struct fat16_file *f);

/* Write n bytes at f->pos (at the end if FAT_O_APPEND).  *put receives the count written.  A short
 * count with FAT_OK means the next call will report the error (disk full, I/O error). */
int fat16_write(struct fat16_file *f, const unsigned char *src, unsigned int n, unsigned int *put);

/* Write the directory entry if size or first cluster changed. */
int fat16_flush(struct fat16_file *f);

/* Set the size.  Shrinking frees clusters; growing appends zero bytes (disk full -> FAT_ERR_FULL,
 * size left where it got to). */
int fat16_truncate(struct fat16_file *f, unsigned long size);

/* Delete a closed file (entry marked deleted, then its chain freed).  A file whose chain does not
 * validate is not deleted (the chain error is returned). */
int fat16_remove(struct fat16_vol *v, const char *name);

/* Rename within the root.  FAT_ERR_EXIST if to exists, FAT_ERR_NOFILE if from does not. */
int fat16_rename(struct fat16_vol *v, const char *from, const char *to);

/* Where a root file's directory entry is (sector and byte offset), or FAT_ERR_NOFILE.  Used by the
 * volume driver to refuse a second writer, a delete or a rename of an open file. */
int fat16_lookup(struct fat16_vol *v, const char *name, unsigned int *lba, unsigned int *off);

/* Count free clusters (scans the FAT). */
int fat16_free_clusters(struct fat16_vol *v, unsigned long *nfree);

/* ---- R11M: subdirectories and paths ------------------------------------------------------------
 * A directory is identified by its first cluster; 0 is the root (the fixed region).  A path is
 * "A/B/C.EXT": components separated by '/', a leading '/' starts at the root, otherwise at the
 * directory given as cwd.  "." and ".." are understood ("..": the parent, the root stays the root).
 * Empty components are ignored.  At most FAT_PATH_MAX characters, FAT_PATH_DEPTH components.  Names
 * are 8.3 (A-Z 0-9 _ and one dot); with FATP_CI lower case is folded first.  The empty path is the
 * directory cwd itself. */
#define FAT_PATH_MAX   64u
#define FAT_PATH_DEPTH 16u
#define FATP_CI        0x01u     /* fold a-z to A-Z in names */

#define FATL_DIRS      0x01u     /* listing: include subdirectories (record size field 0xFFFFFFFF) */
#define FATL_LONG      0x02u     /* listing: long records (R11M VFAT; see win1.c) */

#ifdef QDOS_VFAT
/* ---- R11M VFAT long file names (-DQDOS_VFAT, needs QDOS_FS_WRITE) -----------------------------------
 * Policy (docs/R11M-VFAT-LIMITATIONS.md): a name that is a valid short name once upper-cased (base 1..8, extension 0..3,
 * letters, digits and ! # $ % & ' ( ) - @ ^ _ ` { } ~) is stored as a short entry only, upper case, exactly as before.
 * Any other name of 1..FAT_LFN_MAX printable ASCII characters (not / \ : * ? " < > |, no leading space, no trailing space
 * or dot) is stored with a VFAT long-name chain and a generated 8.3 alias (BASE~N.EXT).  Long names on disk that are
 * not representable in that subset (non-ASCII, longer, malformed) are never matched by long name; their alias works. */
#define FAT_LFN_MAX 63u
#endif

struct fat16_path {
	unsigned int dir;            /* directory that holds the last component (or the directory itself) */
	unsigned char name[11];      /* last component, 8.3 form (valid when has_name and short_ok) */
	int has_name;                /* 0: the path names directory 'dir' itself ("/", ".", "A/..", "") */
	int trailing_slash;
#ifdef QDOS_VFAT
	int short_ok;                /* the last component is a valid short name (name[] is set) */
	unsigned int llen;           /* the last component as typed (long-name candidate) */
	char lname[FAT_LFN_MAX + 1];
#endif
};

/* What a lookup found. */
struct fat16_info {
	unsigned int attr;           /* attribute byte (0x10 = directory) */
	unsigned int first;          /* first cluster */
	unsigned long size;
	unsigned int lba, off;       /* where the directory entry is */
	unsigned int dir;            /* directory that holds it */
};

int fat16_resolve(struct fat16_vol *v, unsigned int cwd, const char *path, unsigned int pf,
                  struct fat16_path *out);
/* Look a path up.  FAT_ERR_NOFILE if absent.  A path that names a directory itself ("/", ".", "..")
 * reports attr 0x10 and first = that directory, with off = 0xFFFF and lba = 0. */
int fat16_stat_path(struct fat16_vol *v, unsigned int cwd, const char *path, unsigned int pf,
                    struct fat16_info *info);
/* Open by path.  A directory (opened FAT_O_READ only) gives a listing cursor (f->is_dir). */
int fat16_open_path(struct fat16_vol *v, unsigned int cwd, const char *path, unsigned int pf,
                    unsigned int flags, struct fat16_file *f);
int fat16_dir_list_open(struct fat16_vol *v, unsigned int dir, unsigned int lflags, struct fat16_file *f);
/* Next entry of a listing: 1 and *out, 0 at the end, or a negative error. */
int fat16_dir_list_next(struct fat16_file *f, struct fat16_dirent *out);
int fat16_mkdir(struct fat16_vol *v, unsigned int cwd, const char *path, unsigned int pf);
/* rmdir: the directory must hold nothing but '.' and '..'.  The caller refuses the current directory
 * of any job (it knows them); this layer refuses the root, '.' and '..' (FAT_ERR_INVAL). */
int fat16_rmdir(struct fat16_vol *v, unsigned int cwd, const char *path, unsigned int pf);
int fat16_remove_path(struct fat16_vol *v, unsigned int cwd, const char *path, unsigned int pf);
/* Rename or move.  Files may move between directories; a directory only within its parent. */
int fat16_rename_path(struct fat16_vol *v, unsigned int cwd, const char *from, const char *to,
                      unsigned int pf);
/* The directory a path names, as a cluster (0 = root); FAT_ERR_NOTDIR if it is a file. */
int fat16_chdir_path(struct fat16_vol *v, unsigned int cwd, const char *path, unsigned int pf,
                     unsigned int *newdir);
/* The absolute path of directory dir, found by walking ".." ("/" for the root). */
int fat16_getcwd(struct fat16_vol *v, unsigned int dir, char *buf, unsigned int n);
#endif

#endif
