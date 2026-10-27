/*
 * QDOS/386 R8 - streaming RUN and boot loads.  See imgload.h.
 */
#include "imgload.h"
#include "aout.h"
#include "channel.h"
#include "fat16.h"
#include "win1.h"
#ifdef QDOS_PAGING_POC
#include "paging.h"
#endif

#define IMG_NAME_MAX 18u           /* name, NUL included: full 8.3 (R11E) */

/* Name plus arguments, as the kernel passes them to the loader.  Kernel static:
 * only the trap that fills it uses it, and traps do not nest. */
static unsigned char img_packed[APP_ARG_STR_MAX];

/* Byte source for the loader: a FAT16 file, read at any offset. */
struct fat_src {
	struct fat16_file *f;
};

static int fat_rd(void *ctx, unsigned int off, unsigned char *dst, unsigned int n)
{
	struct fat_src *s = (struct fat_src *) ctx;
	unsigned int got = 0;
	int rc;

	if (n == 0)
		return APP_OK;
	rc = fat16_seek(s->f, (unsigned long) off);
	if (rc != FAT_OK)
		return rc;
	while (n > 0) {
		rc = fat16_read(s->f, dst, n, &got);
		if (rc != FAT_OK)
			return rc;
		if (got == 0)                    /* chain shorter than the size */
			return FAT_ERR_SIZE;
		dst += got;
		n -= got;
	}
	return APP_OK;
}

/* Copy a user name (at most IMG_NAME_MAX-1 characters). */
static int copy_user_name(unsigned int addr, char *out)
{
	unsigned int i;
	for (i = 0; i < IMG_NAME_MAX; i++) {
		if (!chan_user_range_ok(addr + i, 1))
			return 0;
		out[i] = *(char *) (addr + i);
		if (out[i] == 0)
			return 1;
	}
	return 0;
}

/* Load the file called name into want_slot, with packed as its arguments. */
static int img_load_named(const char *name, int want_slot,
                          const unsigned char *packed, unsigned int plen)
{
	struct fat16_vol *v;
	struct fat16_file f;
	struct fat_src src;
	int rc;

	v = win1_volume();
	if (v == 0)
		return CHAN_ERR_NOVOL;
	rc = fat16_open_root(v, name, &f);
	if (rc != FAT_OK)
		return rc;
	if (f.size == 0)
		return APP_ERR_SIZE;
	if (f.size > IMG_FILE_MAX)
		return APP_ERR_TOOBIG;
	src.f = &f;
	return app_load_src(fat_rd, &src, (unsigned int) f.size, want_slot, packed, plen);
}

int img_load_kernel(const char *name, unsigned int want_slot)
{
	unsigned int n = 0;

	while (name[n] != 0 && n < IMG_NAME_MAX - 1)
		n++;
	if (name[n] != 0)
		return FAT_ERR_NAME;
	/* argv = { name }: the name, its NUL. */
	{
		unsigned int i;
		for (i = 0; i <= n; i++)
			img_packed[i] = (unsigned char) name[i];
		return img_load_named(name, want_slot, img_packed, n + 1);
	}
}

int img_load_request(unsigned int name_addr, unsigned int args_addr, unsigned int args_len)
{
	char name[IMG_NAME_MAX];
	unsigned int n = 0, i, total;

	if (!copy_user_name(name_addr, name))
		return FAT_ERR_NAME;
	while (name[n] != 0)
		n++;
	if (args_len == 0) {
		for (i = 0; i <= n; i++)
			img_packed[i] = (unsigned char) name[i];
		return img_load_named(name, 1, img_packed, n + 1);
	}
	if (args_len > APP_ARG_STR_MAX || n + 1 + args_len > APP_ARG_STR_MAX)
		return APP_ERR_ARGS;
	if (!chan_user_range_ok(args_addr, args_len))
		return CHAN_ERR_RANGE;
	if (*(unsigned char *) (args_addr + args_len - 1) != 0)
		return APP_ERR_ARGS;          /* arguments must end with NUL */
	for (i = 0; i <= n; i++)
		img_packed[i] = (unsigned char) name[i];
	total = n + 1;
	for (i = 0; i < args_len; i++)
		img_packed[total + i] = *(unsigned char *) (args_addr + i);
	total += args_len;
	return img_load_named(name, 1, img_packed, total);
}

/* R11H: start the named WIN1_ root file at a profile (0..3), or any (-1), with
 * no arguments.  Returns the tagged job id, or a negative code. */
int img_start_request(unsigned int name_addr, int profile)
{
	char name[IMG_NAME_MAX];
	unsigned int n = 0, i;

	if (profile < -1 || profile >= APP_PROFILES)
		return APP_ERR_NOPROFILE;
	if (!copy_user_name(name_addr, name))
		return FAT_ERR_NAME;
	while (name[n] != 0)
		n++;
	for (i = 0; i <= n; i++)
		img_packed[i] = (unsigned char) name[i];
	return img_load_named(name, profile, img_packed, n + 1);
}

#ifdef QDOS_PAGING_POC
/* R11J: QT_PG_START.  The named WIN1_ root file, linked for slot 1, is loaded into a new
 * address space: private frames at the common window PG_VBASE.  Arguments as for RUN. */
int img_pg_start_request(unsigned int name_addr, unsigned int args_addr, unsigned int args_len)
{
	char name[IMG_NAME_MAX];
	unsigned int n = 0, i, total;
	struct fat16_vol *v;
	struct fat16_file f;
	struct fat_src src;
	int rc;

	if (!copy_user_name(name_addr, name))
		return FAT_ERR_NAME;
	while (name[n] != 0)
		n++;
	if (args_len > APP_ARG_STR_MAX || n + 1 + args_len > APP_ARG_STR_MAX)
		return APP_ERR_ARGS;
	if (args_len != 0) {
		if (!chan_user_range_ok(args_addr, args_len))
			return CHAN_ERR_RANGE;
		if (*(unsigned char *) (args_addr + args_len - 1) != 0)
			return APP_ERR_ARGS;
	}
	for (i = 0; i <= n; i++)
		img_packed[i] = (unsigned char) name[i];
	total = n + 1;
	for (i = 0; i < args_len; i++)
		img_packed[total + i] = *(unsigned char *) (args_addr + i);
	total += args_len;

	v = win1_volume();
	if (v == 0)
		return CHAN_ERR_NOVOL;
	rc = fat16_open_root(v, name, &f);
	if (rc != FAT_OK)
		return rc;
	if (f.size == 0)
		return APP_ERR_SIZE;
	if (f.size > IMG_FILE_MAX)
		return APP_ERR_TOOBIG;
	src.f = &f;
	return app_load_paged(fat_rd, &src, (unsigned int) f.size, img_packed, total);
}
#endif
