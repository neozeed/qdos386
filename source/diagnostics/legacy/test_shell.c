/*
 * QDOS/386 R7 - the shell self-test in the regression build.
 *
 * SHELLT.AOU is read from the WIN1_ volume and loaded into slot 0 by the
 * R5 loader.  It runs its own guest self-test (apps/shell/shell.c) and exits
 * with status 0 only when every check passed.  The kernel only checks the
 * load, the slot and that status.  No key is injected.
 */
#include "platform.h"
#include "jobs.h"
#include "aout.h"
#include "fat16.h"
#include "win1.h"
#include "test_channels.h"
#include "test_shell.h"

#define SH_IMG_MAX 8192u
#define SH_OK_STATUS 0

static unsigned char shimg[SH_IMG_MAX];

void shell_phase(void)
{
	struct fat16_vol *v = win1_volume();
	struct fat16_file f;
	unsigned int off = 0, got = 0;
	int rc, id, st;

	if (v == 0 || fat16_open_root(v, "SHELLT.AOU", &f) != FAT_OK ||
	    f.size == 0 || f.size > SH_IMG_MAX) {
		r4_add("SHELLT.AOU: read from the volume", 0);
		return;
	}
	while (off < (unsigned int) f.size) {
		rc = fat16_read(&f, shimg + off, (unsigned int) f.size - off, &got);
		if (rc != FAT_OK || got == 0)
			break;
		off += got;
	}
	r4_add("SHELLT.AOU: read from the volume", off == (unsigned int) f.size);

	id = app_load_bytes(shimg, (unsigned int) f.size);
	r4_add("SHELLT.AOU: loaded into slot 0", id > 0 && app_slot_owner(0) == id);
	if (id <= 0)
		return;

	for (;;) {
		st = qtrap(QT_JOB_EXIT, (unsigned int) id, 0, 0);
		if (st != JOB_ERR_RUNNING)
			break;
		qtrap(QT_SUSPEND, 1, 0, 0);
	}
	while (jobs[id & 0xffff].in_use)
		qtrap(QT_SUSPEND, 1, 0, 0);
	r4_add("SHELLT.AOU: every guest self-test check passed (exit status 0)",
	       st == SH_OK_STATUS);
	r4_add("SHELLT.AOU: slot 0 released after the shell job", app_slot_owner(0) == 0);
}
