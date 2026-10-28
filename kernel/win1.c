/*
 * QDOS/386 R6 - WIN1_ file channels.  The driver is a prefix driver: the
 * channel name "WIN1_" selects it, and the rest of the name is the root
 * directory file name.  Read and seek only; write is not supported.
 */
#include "platform.h"
#include "channel.h"
#include "fat16.h"
#include "win1.h"

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
	/* The driver was selected by its prefix, so the file name follows it.
	 * An empty name ("WIN1_") is the root directory (R7): read it as 16-byte
	 * records, see fat16_dirent. */
	if (name[WIN1_PREFIX_LEN] == 0)
		return fat16_dir_open(&vol, &ch->ff);
#ifdef QDOS_FS_WRITE
	{
		unsigned int flags = ch->mode & (CHAN_O_READ | CHAN_O_WRITE | CHAN_O_CREATE | CHAN_O_TRUNC |
		                                 CHAN_O_APPEND | CHAN_O_EXCL);
		unsigned int lba = 0, off = 0;
		int rc, writer = (flags & CHAN_O_WRITE) != 0;

		if (flags == 0 || (ch->mode & ~0x3fu) != 0)
			return CHAN_ERR_RANGE;
		if (!writer && (flags & (CHAN_O_CREATE | CHAN_O_TRUNC | CHAN_O_APPEND | CHAN_O_EXCL)))
			return CHAN_ERR_RANGE;
		/* conflict check BEFORE anything is created or truncated */
		rc = fat16_lookup(&vol, name + WIN1_PREFIX_LEN, &lba, &off);
		if (rc == FAT_OK && openf_busy(lba, off) && (writer || openf[openf_find(lba, off)].writers != 0))
			return FAT_ERR_BUSY;
		if (rc != FAT_OK && rc != FAT_ERR_NOFILE)
			return rc;
		rc = fat16_open_ex(&vol, name + WIN1_PREFIX_LEN, flags, &ch->ff);
		if (rc != FAT_OK)
			return rc;
		rc = openf_add((unsigned int) ch->ff.dir_lba, ch->ff.dir_off, writer);
		if (rc != 0)
			return rc;
		return FAT_OK;
	}
#else
	return fat16_open_root(&vol, name + WIN1_PREFIX_LEN, &ch->ff);
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

/* Directory channel: whole 16-byte records only.  n must hold at least one
 * record; the count is a multiple of 16, and 0 at the end of the listing. */
static int win1_dir_read(struct chan_object *ch, unsigned char *buf, unsigned int n)
{
	unsigned int got = 0;
	unsigned int i;
	struct fat16_dirent d;
	int rc;

	if (n < WIN1_DIR_RECORD)
		return CHAN_ERR_RANGE;
	while (got + WIN1_DIR_RECORD <= n) {
		rc = fat16_dir_next(&ch->ff, &d);
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
	if (ch->ff.is_dir)
		return op == CHAN_CTL_FLUSH ? 0 : CHAN_ERR_NOTSUP;
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
	unsigned int lba, off;
	unsigned long nfree;
	int rc;

	if (!mounted)
		return CHAN_ERR_NOVOL;
	switch (op) {
	case CHAN_FS_REMOVE:
		if (a[WIN1_PREFIX_LEN] == 0)
			return FAT_ERR_NAME;
		rc = fat16_lookup(&vol, a + WIN1_PREFIX_LEN, &lba, &off);
		if (rc != FAT_OK)
			return rc;
		if (openf_busy(lba, off))
			return FAT_ERR_BUSY;
		return fat16_remove(&vol, a + WIN1_PREFIX_LEN);
	case CHAN_FS_RENAME:
		if (a[WIN1_PREFIX_LEN] == 0 || b[WIN1_PREFIX_LEN] == 0)
			return FAT_ERR_NAME;
		rc = fat16_lookup(&vol, a + WIN1_PREFIX_LEN, &lba, &off);
		if (rc != FAT_OK)
			return rc;
		if (openf_busy(lba, off))
			return FAT_ERR_BUSY;
		return fat16_rename(&vol, a + WIN1_PREFIX_LEN, b + WIN1_PREFIX_LEN);
	case CHAN_FS_FREE:
		rc = fat16_free_clusters(&vol, &nfree);
		return rc != FAT_OK ? rc : (int) nfree;
	case CHAN_FS_CLUSTER:
		return (int) (vol.spc * FAT_SECTOR_BYTES);
	case CHAN_FS_TOTAL:
		return (int) vol.clusters;
	}
	return CHAN_ERR_NOTSUP;
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
