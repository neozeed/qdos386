/*
 * QDOS/386 R6 - WIN1_ file channels.  The driver is a prefix driver: the
 * channel name "WIN1_" selects it, and the rest of the name is the root
 * directory file name.  Read and seek only; write is not supported.
 */
#include "platform.h"
#include "channel.h"
#include "fat16.h"
#include "win1.h"

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
	/* The driver was selected by its prefix, so the file name follows it. */
	return fat16_open_root(&vol, name + WIN1_PREFIX_LEN, &ch->ff);
}

static void win1_close(struct chan_object *ch)
{
	(void) ch;
}

static int win1_read(struct chan_object *ch, unsigned char *buf, unsigned int n)
{
	unsigned int got = 0;
	int rc = fat16_read(&ch->ff, buf, n, &got);
	if (rc != FAT_OK)
		return rc;
	return (int) got;
}

static int win1_seek(struct chan_object *ch, unsigned long pos)
{
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
