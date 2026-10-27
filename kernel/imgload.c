/*
 * QDOS/386 R7 - QT_IMG_LOAD.  See imgload.h.
 */
#include "imgload.h"
#include "aout.h"
#include "channel.h"
#include "fat16.h"
#include "memory.h"
#include "win1.h"

#define IMG_NAME_MAX 16u           /* name, NUL included */

/* Copy a NUL-terminated user name (at most IMG_NAME_MAX-1 characters). */
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

/* The loader proper.  Name is a kernel string, at most IMG_NAME_MAX-1 bytes. */
static int img_load_named(const char *name, unsigned int want_slot)
{
	struct fat16_vol *v;
	struct fat16_file f;
	struct app_image info;
	unsigned char *buf;
	unsigned int off = 0, got = 0;
	int rc, id;

	v = win1_volume();
	if (v == 0)
		return CHAN_ERR_NOVOL;

	rc = fat16_open_root(v, name, &f);
	if (rc != FAT_OK)
		return rc;
	if (f.size == 0)
		return APP_ERR_SIZE;
	if (f.size > IMG_MAX)
		return APP_ERR_TOOBIG;

	buf = (unsigned char *) mem_alloc((unsigned int) f.size, 0);
	if (mem_is_error((int) (unsigned int) buf))
		return APP_ERR_NOMEM;

	while (off < (unsigned int) f.size) {
		rc = fat16_read(&f, buf + off, (unsigned int) f.size - off, &got);
		if (rc != FAT_OK) {
			mem_free(buf, 0);
			return rc;
		}
		if (got == 0) {                 /* chain shorter than the size */
			mem_free(buf, 0);
			return FAT_ERR_SIZE;
		}
		off += got;
	}

	rc = aout_check(buf, (unsigned int) f.size, &info);
	if (rc != APP_OK) {
		mem_free(buf, 0);
		return rc;
	}
	if (info.slot != want_slot) {
		mem_free(buf, 0);
		return (want_slot == 1) ? APP_ERR_NOTSLOT1 : APP_ERR_NOTSLOT0;
	}
	id = app_load_bytes(buf, (unsigned int) f.size);
	mem_free(buf, 0);
	return id;
}

int img_load_kernel(const char *name, unsigned int want_slot)
{
	return img_load_named(name, want_slot);
}

int img_load_request(unsigned int name_addr)
{
	char name[IMG_NAME_MAX];
	int rc;

	if (!copy_user_name(name_addr, name))
		return FAT_ERR_NAME;
	rc = img_load_named(name, 1);   /* RUN loads into slot 1 only */
	return rc;
}
