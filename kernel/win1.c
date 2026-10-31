/*
 * QDOS/386 R6 - WIN1_ file channels.  The driver is a prefix driver: the
 * channel name "WIN1_" selects it, and the rest of the name is the root
 * directory file name.  Read and seek only; write is not supported.
 */
#include "platform.h"
#include "channel.h"
#include "fat16.h"
#include "win1.h"
#ifdef QDOS_FS_WRITE
#include "jobs.h"
#endif

/* chan_object.ff: see channel.h; for the directory channel the same field
 * holds the listing cursor (fat16_file.is_dir). */

#define WIN1_PREFIX "WIN1_"
#define WIN1_PREFIX_LEN 5

static struct fat16_vol vol;
static int mounted = 0;

static int disk_thunk(void *ctx, unsigned int lba, unsigned char *buf)
{
	(void) ctx;
	return disk_read_sector(lba, buf);
}

#ifdef QDOS_FS_WRITE
static int disk_thunk_w(void *ctx, unsigned int lba, const unsigned char *buf)
{
	(void) ctx;
	return disk_write_sector(lba, buf);
}

/* Open-file registry: one record per distinct directory entry in use.  A file may have any number
 * of readers or exactly one writer, never both; delete and rename need the file closed.  The key is
 * the entry's sector and offset, so two names cannot alias one file.  Entries are released by
 * win1_close, which also runs for every channel of an exiting job. */
#define OPENF_MAX CHAN_MAX
static struct {
	unsigned int lba, off;
	unsigned int readers, writers;
} openf[OPENF_MAX];

static int openf_find(unsigned int lba, unsigned int off)
{
	int i;
	for (i = 0; i < OPENF_MAX; i++)
		if ((openf[i].readers | openf[i].writers) != 0 && openf[i].lba == lba && openf[i].off == off)
			return i;
	return -1;
}

static int openf_busy(unsigned int lba, unsigned int off)
{
	return openf_find(lba, off) >= 0;
}

/* Register an open.  Returns 0, or FAT_ERR_BUSY on a conflict, FAT_ERR_RANGE if the table is full. */
static int openf_add(unsigned int lba, unsigned int off, int writer)
{
	int i = openf_find(lba, off);
	if (i >= 0) {
		if (writer || openf[i].writers != 0)
			return FAT_ERR_BUSY;
		openf[i].readers++;
		return 0;
	}
	for (i = 0; i < OPENF_MAX; i++)
		if ((openf[i].readers | openf[i].writers) == 0)
			break;
	if (i >= OPENF_MAX)
		return FAT_ERR_RANGE;
	openf[i].lba = lba;
	openf[i].off = off;
	openf[i].readers = writer ? 0 : 1;
	openf[i].writers = writer ? 1 : 0;
	return 0;
}

static void openf_del(unsigned int lba, unsigned int off, int writer)
{
	int i = openf_find(lba, off);
	if (i < 0)
		return;
	if (writer) {
		if (openf[i].writers != 0)
			openf[i].writers--;
	} else if (openf[i].readers != 0) {
		openf[i].readers--;
	}
}
#endif


#ifdef QDOS_FS_WRITE
/* ---- R11M: current directory per job and path forms -------------------------------------------
 * cwdtab[slot] holds the current directory (first cluster; 0 = root) of the job that owns the slot,
 * valid only while owner equals the slot's current tagged id; a reused slot starts at the root.
 * A new job inherits its creator's directory (win1_cwd_inherit, called from job_alloc_sized). */
static struct {
	unsigned int owner;
	unsigned int dir;
} cwdtab[JOB_MAX];

static unsigned int cwd_of(unsigned int id)
{
	unsigned int slot = id & 0xffffu;
	if (slot < JOB_MAX && cwdtab[slot].owner == id)
		return cwdtab[slot].dir;
	return 0;
}

static unsigned int cur_cwd(void)
{
#ifdef QDOS_R13B
	return cwd_of(jobs[JOB_CTX_SLOT()].id);        /* R13B: a kernel launch searches from its context's directory */
#else
	return cwd_of(job_current_id());
#endif
}

static void cwd_set(unsigned int dir)
{
	unsigned int id = job_current_id();
	unsigned int slot = id & 0xffffu;
	if (slot < JOB_MAX) {
		cwdtab[slot].owner = id;
		cwdtab[slot].dir = dir;
	}
}

void win1_cwd_inherit(unsigned int child_id, unsigned int parent_id)
{
	unsigned int slot = child_id & 0xffffu;
	if (slot < JOB_MAX) {
		cwdtab[slot].owner = child_id;
		cwdtab[slot].dir = cwd_of(parent_id);
	}
}

/* Is any live job's current directory dir? */
static int cwd_in_use(unsigned int dir)
{
	unsigned int i;
	for (i = 0; i < JOB_MAX; i++)
		if (cwdtab[i].dir == dir && cwdtab[i].owner != 0 && job_lookup(cwdtab[i].owner) >= 0)
			return 1;
	return 0;
}

#define WIN1_PATH_BUF (FAT_PATH_MAX + 4)

/* Turn a channel name ("WIN1_..." or "WIN1:...") into a path that fat16_resolve reads relative to
 * the calling job's current directory, and the path flags.  Legacy "WIN1_NAME" (no '/') is the flat
 * root name of R6..R11L: uppercase 8.3 only, always in the root.  "WIN1_A/B" is a root path.
 * "WIN1:..." is a native path, relative to the current directory unless it starts with '/'. */
static int win1_norm(const char *name, char *out, unsigned int *pf)
{
	const char *rest = name + WIN1_PREFIX_LEN;
	unsigned int n = 0, i, slash = 0, n_rest;

	for (i = 0; rest[i] != 0; i++) {
		if (i >= FAT_PATH_MAX)
			return FAT_ERR_DEPTH;
		if (rest[i] == '/')
			slash = 1;
	}
	n_rest = i;
	*pf = FATP_CI;
	if (name[4] == '_') {
		if (!slash && rest[0] != 0) {
			unsigned char t[11];
			int rc;

			if (n_rest > 12u)            /* R11E/R11L: "WIN1_" + more than a full 8.3 name is out of range (-14), not a bad name */
				return CHAN_ERR_RANGE;
			rc = fat16_name83(rest, t);
			if (rc != FAT_OK)
				return rc;
			*pf = 0;
		}
		out[n++] = '/';                  /* root-anchored (also for the bare prefix: the root) */
	}
	for (i = 0; rest[i] != 0; i++)
		out[n++] = rest[i];
	out[n] = 0;
	return FAT_OK;
}
#endif

int win1_mount(void)
{
	int rc;
	if (mounted)
		return FAT_OK;
	rc = fat16_mount(&vol, disk_thunk, 0, disk_sector_count());
	if (rc == FAT_OK) {
		mounted = 1;
#ifdef QDOS_FS_WRITE
		fat16_set_writer(&vol, disk_thunk_w);
#endif
	}
	return rc;
}

struct fat16_vol *win1_volume(void)
{
	return mounted ? &vol : 0;
}

static int win1_open(struct chan_object *ch, const char *name)
{
	if (!mounted)
		return CHAN_ERR_NOVOL;
#ifndef QDOS_FS_WRITE
	/* The driver was selected by its prefix, so the file name follows it.
	 * An empty name ("WIN1_") is the root directory (R7): read it as 16-byte
	 * records, see fat16_dirent. */
	if (name[WIN1_PREFIX_LEN] == 0)
		return fat16_dir_open(&vol, &ch->ff);
	return fat16_open_root(&vol, name + WIN1_PREFIX_LEN, &ch->ff);
#else
	{
		unsigned int flags = ch->mode & (CHAN_O_READ | CHAN_O_WRITE | CHAN_O_CREATE | CHAN_O_TRUNC |
		                                 CHAN_O_APPEND | CHAN_O_EXCL | CHAN_O_DIRS | CHAN_O_FILE);
		unsigned int pf = 0, cwd, lflags;
		char path[WIN1_PATH_BUF];
		struct fat16_info info;
		int rc, writer = (flags & CHAN_O_WRITE) != 0, fileonly;

		if ((flags & ~(CHAN_O_DIRS | CHAN_O_FILE)) == 0 || (ch->mode & ~0xffu) != 0)
			return CHAN_ERR_RANGE;
		if (!writer && (flags & (CHAN_O_CREATE | CHAN_O_TRUNC | CHAN_O_APPEND | CHAN_O_EXCL)))
			return CHAN_ERR_RANGE;
		if ((writer || (flags & CHAN_O_FILE)) && (flags & CHAN_O_DIRS))
			return CHAN_ERR_RANGE;
		lflags = (flags & CHAN_O_DIRS) ? FATL_DIRS : 0;
		flags &= ~(CHAN_O_DIRS | CHAN_O_FILE);
		fileonly = (ch->mode & CHAN_O_FILE) != 0;
		if (name[4] == '_' && name[WIN1_PREFIX_LEN] == 0)     /* legacy "WIN1_": the root listing, files only */
			return fat16_dir_list_open(&vol, 0, 0, &ch->ff);
		rc = win1_norm(name, path, &pf);
		if (rc != FAT_OK)
			return rc;
		cwd = cur_cwd();
		/* conflict check BEFORE anything is created or truncated */
		rc = fat16_stat_path(&vol, cwd, path, pf, &info);
		if (rc == FAT_OK && (info.attr & 0x10)) {
			if (pf == 0)                  /* legacy flat name: directories were never visible */
				return FAT_ERR_NOFILE;
			if (writer || fileonly)
				return FAT_ERR_ISDIR;
			return fat16_dir_list_open(&vol, info.first, lflags, &ch->ff);
		}
		if (rc == FAT_OK && (lflags & FATL_DIRS) && !(info.attr & 0x10))
			return FAT_ERR_NOTDIR;        /* a listing was asked for and the target is a file */
		if (rc == FAT_OK && openf_busy(info.lba, info.off) &&
		    (writer || openf[openf_find(info.lba, info.off)].writers != 0))
			return FAT_ERR_BUSY;
		if (rc != FAT_OK && rc != FAT_ERR_NOFILE)
			return rc;
		rc = fat16_open_path(&vol, cwd, path, pf, flags, &ch->ff);
		if (rc != FAT_OK)
			return rc;
		if (ch->ff.is_dir) {
			ch->ff.lflags = lflags;
			return FAT_OK;
		}
		rc = openf_add((unsigned int) ch->ff.dir_lba, ch->ff.dir_off, writer);
		if (rc != 0)
			return rc;
		return FAT_OK;
	}
#endif
}

static void win1_close(struct chan_object *ch)
{
#ifdef QDOS_FS_WRITE
	if (!ch->ff.is_dir)
		openf_del((unsigned int) ch->ff.dir_lba, ch->ff.dir_off, (ch->ff.mode & FAT_O_WRITE) != 0);
#else
	(void) ch;
#endif
}

#define WIN1_DIR_RECORD 16u
#define WIN1_LDIR_RECORD 96u

#ifdef QDOS_VFAT
/* Long listing records (after CHAN_CTL_LISTLONG), 96 bytes each, little-endian:
 *   0..63   name: the long name if the entry has a representable one, else the 8.3 name; NUL-terminated
 *   64..79  the 8.3 alias, NUL-padded
 *   80..83  size (0xFFFFFFFF for a subdirectory)
 *   84..87  attribute byte
 *   88..91  1: name is a long name; 2: the entry has a long name that cannot be shown (alias given); 0: none
 *   92..95  reserved, 0 */
static void put32le(unsigned char *p, unsigned long v)
{
	p[0] = (unsigned char) (v & 0xff);
	p[1] = (unsigned char) ((v >> 8) & 0xff);
	p[2] = (unsigned char) ((v >> 16) & 0xff);
	p[3] = (unsigned char) ((v >> 24) & 0xff);
}

static int win1_ldir_read(struct chan_object *ch, unsigned char *buf, unsigned int n)
{
	unsigned int got = 0, i;
	struct fat16_ldirent d;
	int rc;

	if (n < WIN1_LDIR_RECORD)
		return CHAN_ERR_RANGE;
	while (got + WIN1_LDIR_RECORD <= n) {
		rc = fat16_dir_list_nextl(&ch->ff, &d);
		if (rc < 0)
			return got != 0 ? (int) got : rc;
		if (rc == 0)
			break;
		for (i = 0; i < 64; i++)
			buf[got + i] = (unsigned char) d.lname[i];
		buf[got + 63] = 0;
		for (i = 0; i < 12; i++)
			buf[got + 64 + i] = (unsigned char) d.name[i];
		for (i = 76; i < 80; i++)
			buf[got + i] = 0;
		put32le(buf + got + 80, d.size);
		put32le(buf + got + 84, d.attr);
		put32le(buf + got + 88, d.has_lfn);
		put32le(buf + got + 92, 0);
		got += WIN1_LDIR_RECORD;
	}
	return (int) got;
}
#endif

/* Directory channel: whole 16-byte records only.  n must hold at least one
 * record; the count is a multiple of 16, and 0 at the end of the listing. */
static int win1_dir_read(struct chan_object *ch, unsigned char *buf, unsigned int n)
{
	unsigned int got = 0;
	unsigned int i;
	struct fat16_dirent d;
	int rc;

#ifdef QDOS_VFAT
	if (ch->ff.lflags & FATL_LONG)
		return win1_ldir_read(ch, buf, n);
#endif
	if (n < WIN1_DIR_RECORD)
		return CHAN_ERR_RANGE;
	while (got + WIN1_DIR_RECORD <= n) {
#ifdef QDOS_FS_WRITE
		rc = fat16_dir_list_next(&ch->ff, &d);
#else
		rc = fat16_dir_next(&ch->ff, &d);
#endif
		if (rc < 0)
			return got != 0 ? (int) got : rc;
		if (rc == 0)
			break;
		for (i = 0; i < 12; i++)
			buf[got + i] = (unsigned char) d.name[i];
		buf[got + 12] = (unsigned char) (d.size & 0xff);
		buf[got + 13] = (unsigned char) ((d.size >> 8) & 0xff);
		buf[got + 14] = (unsigned char) ((d.size >> 16) & 0xff);
		buf[got + 15] = (unsigned char) ((d.size >> 24) & 0xff);
		got += WIN1_DIR_RECORD;
	}
	return (int) got;
}

static int win1_read(struct chan_object *ch, unsigned char *buf, unsigned int n)
{
	unsigned int got = 0;
	int rc;

	if (ch->ff.is_dir)
		return win1_dir_read(ch, buf, n);
	rc = fat16_read(&ch->ff, buf, n, &got);
	if (rc != FAT_OK)
		return rc;
	return (int) got;
}

static int win1_seek(struct chan_object *ch, unsigned long pos)
{
	if (ch->ff.is_dir) {
		/* the listing can only be rewound to its start */
		if (pos != 0)
			return FAT_ERR_RANGE;
		ch->ff.dir_next = 0;
		return FAT_OK;
	}
	return fat16_seek(&ch->ff, pos);
}

#ifdef QDOS_FS_WRITE
static int win1_write(struct chan_object *ch, const unsigned char *buf, unsigned int n)
{
	unsigned int put = 0;
	int rc;

	if (ch->ff.is_dir)
		return CHAN_ERR_NOTSUP;
	/* a channel not opened for writing answers NOTSUP exactly as the read-only R11K driver did (the R11E QDOSAPI test
	   expects -15); -80 (write-protected) stays for a write-capable channel on a read-only device */
	if (!(ch->ff.mode & FAT_O_WRITE))
		return CHAN_ERR_NOTSUP;
	rc = fat16_write(&ch->ff, buf, n, &put);
	if (rc != FAT_OK)
		return rc;
	return (int) put;
}

static int win1_ctl(struct chan_object *ch, unsigned int op, unsigned long arg)
{
	if (ch->ff.is_dir) {
#ifdef QDOS_VFAT
		if (op == CHAN_CTL_LISTLONG) {
			ch->ff.lflags |= FATL_LONG;
			return 0;
		}
#endif
		return op == CHAN_CTL_FLUSH ? 0 : CHAN_ERR_NOTSUP;
	}
	switch (op) {
	case CHAN_CTL_FLUSH:
		return fat16_flush(&ch->ff);
	case CHAN_CTL_TRUNC:
		if (!(ch->ff.mode & FAT_O_WRITE))
			return FAT_ERR_WPROT;
		return fat16_truncate(&ch->ff, arg);
	case CHAN_CTL_SIZE:
		return ch->ff.size > 0x7fffffffUL ? FAT_ERR_RANGE : (int) ch->ff.size;
	case CHAN_CTL_TELL:
		return ch->ff.pos > 0x7fffffffUL ? FAT_ERR_RANGE : (int) ch->ff.pos;
	}
	return CHAN_ERR_NOTSUP;
}

static int win1_fsop(unsigned int op, const char *a, const char *b)
{
	struct fat16_info info;
	char pa[WIN1_PATH_BUF], pb[WIN1_PATH_BUF];
	unsigned int pf = 0, pf2 = 0, cwd;
	unsigned long nfree;
	int rc;

	if (!mounted)
		return CHAN_ERR_NOVOL;
	cwd = cur_cwd();
	switch (op) {
	case CHAN_FS_GETCWD:
		/* a = output buffer, b = size (raw values; channel.c checked the range) */
		return fat16_getcwd(&vol, cwd, (char *) a, (unsigned int) (unsigned long) b);
	case CHAN_FS_FREE:
		rc = fat16_free_clusters(&vol, &nfree);
		return rc != FAT_OK ? rc : (int) nfree;
	case CHAN_FS_CLUSTER:
		return (int) (vol.spc * FAT_SECTOR_BYTES);
	case CHAN_FS_TOTAL:
		return (int) vol.clusters;
	}
	rc = win1_norm(a, pa, &pf);
	if (rc != FAT_OK)
		return rc;
	switch (op) {
	case CHAN_FS_REMOVE:
		if (pa[0] == 0)
			return FAT_ERR_NAME;
		rc = fat16_stat_path(&vol, cwd, pa, pf, &info);
		if (rc != FAT_OK)
			return rc;
		if (info.attr & 0x10)
			return FAT_ERR_ISDIR;
		if (openf_busy(info.lba, info.off))
			return FAT_ERR_BUSY;
		return fat16_remove_path(&vol, cwd, pa, pf);
	case CHAN_FS_RENAME:
		if (pa[0] == 0)
			return FAT_ERR_NAME;
		rc = win1_norm(b, pb, &pf2);
		(void) pf2;
		if (rc != FAT_OK)
			return rc;
		if (pb[0] == 0)
			return FAT_ERR_NAME;
		rc = fat16_stat_path(&vol, cwd, pa, pf, &info);
		if (rc != FAT_OK)
			return rc;
		if (info.off != 0xffffu && openf_busy(info.lba, info.off))
			return FAT_ERR_BUSY;
		return fat16_rename_path(&vol, cwd, pa, pb, FATP_CI);
	case CHAN_FS_MKDIR:
		if (pa[0] == 0)
			return FAT_ERR_NAME;
		return fat16_mkdir(&vol, cwd, pa, pf);
	case CHAN_FS_RMDIR:
		if (pa[0] == 0)
			return FAT_ERR_NAME;
		rc = fat16_stat_path(&vol, cwd, pa, pf, &info);
		if (rc != FAT_OK)
			return rc;
		if (!(info.attr & 0x10))
			return FAT_ERR_NOTDIR;
		if (info.off == 0xffffu)
			return FAT_ERR_INVAL;            /* "/", ".", ".." */
		if (cwd_in_use(info.first))
			return FAT_ERR_BUSY;
		return fat16_rmdir(&vol, cwd, pa, pf);
	case CHAN_FS_CHDIR:
		{
			unsigned int nd = 0;
			rc = fat16_chdir_path(&vol, cwd, pa, pf, &nd);
			if (rc != FAT_OK)
				return rc;
			cwd_set(nd);
			return 0;
		}
	case CHAN_FS_STAT:
		rc = fat16_stat_path(&vol, cwd, pa, pf, &info);
		if (rc != FAT_OK)
			return rc;
		if (b != 0) {
			unsigned char *o = (unsigned char *) b;
			unsigned long sz = (info.attr & 0x10) ? 0ul : info.size;
			o[0] = (unsigned char) (sz & 0xff);
			o[1] = (unsigned char) ((sz >> 8) & 0xff);
			o[2] = (unsigned char) ((sz >> 16) & 0xff);
			o[3] = (unsigned char) ((sz >> 24) & 0xff);
			o[4] = (unsigned char) info.attr;
			o[5] = o[6] = o[7] = 0;
		}
		return (info.attr & 0x10) ? 1 : 0;
	}
	return CHAN_ERR_NOTSUP;
}

/* Kernel-side open of a program image by (cwd-relative, case-folded) path; directories are refused. */
int win1_open_img(const char *name, struct fat16_file *f)
{
	int rc;
	if (!mounted)
		return CHAN_ERR_NOVOL;
	if (name[0] == 'W' && name[1] == 'I' && name[2] == 'N' && name[3] == '1' &&
	    (name[4] == '_' || name[4] == ':')) {
		/* a channel name ("WIN1_NAME" legacy flat, "WIN1:path" native): the same rules as win1_open */
		char path[WIN1_PATH_BUF];
		unsigned int pf = 0;

		rc = win1_norm(name, path, &pf);
		if (rc != FAT_OK)
			return rc;
		rc = fat16_open_path(&vol, cur_cwd(), path, pf, FAT_O_READ, f);
	} else {
		rc = fat16_open_path(&vol, cur_cwd(), name, FATP_CI, FAT_O_READ, f);
	}
	if (rc != FAT_OK)
		return rc;
	if (f->is_dir)
		return FAT_ERR_ISDIR;
	return FAT_OK;
}
#endif

struct chan_driver win1_driver = {
	WIN1_PREFIX,        /* name: the prefix */
	1,                  /* prefix match */
	win1_open,
	win1_close,
#ifdef QDOS_FS_WRITE
	win1_write,         /* R11L: write (fails with FAT_ERR_WPROT on a read-only device or mode) */
#else
	0,                  /* write: not supported (read-only volume) */
#endif
	win1_read,
	win1_seek
#ifdef QDOS_FS_WRITE
	, win1_ctl,
	win1_fsop
#endif
};

#ifdef QDOS_FS_WRITE
/* R11M: the native path form "WIN1:/DIR/NAME" is the same driver under a second prefix. */
struct chan_driver win1n_driver = {
	"WIN1:",
	1,
	win1_open,
	win1_close,
	win1_write,
	win1_read,
	win1_seek,
	win1_ctl,
	win1_fsop
};
#endif
