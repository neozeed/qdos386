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

int win1_mount(void)
{
	int rc;
	if (mounted)
		return FAT_OK;
	rc = fat16_mount(&vol, disk_thunk, 0, disk_sector_count());
	if (rc == FAT_OK)
		mounted = 1;
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
	return fat16_open_root(&vol, name + WIN1_PREFIX_LEN, &ch->ff);
}

static void win1_close(struct chan_object *ch)
{
	(void) ch;
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

struct chan_driver win1_driver = {
	WIN1_PREFIX,        /* name: the prefix */
	1,                  /* prefix match */
	win1_open,
	win1_close,
	0,                  /* write: not supported (read-only volume) */
	win1_read,
	win1_seek
};
