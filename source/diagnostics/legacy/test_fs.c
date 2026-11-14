/*
 * QDOS/386 R6 - FAT16 file phase.
 *
 * Positive: mount WIN1_, open WIN1_HELLO.AOU through the channel manager, read
 * the whole file in chunks, check it, load it with the R5 loader, run it as a
 * preemptive job and check its exit status.
 *
 * Negative: the same FAT16 code on a copy of one sector at a time (the sector
 * source is a wrapper that substitutes one patched sector).  Each case must be
 * refused with the expected error, and the disk itself is never written.
 */
#include "platform.h"
#include "jobs.h"
#include "channel.h"
#include "aout.h"
#include "fat16.h"
#include "win1.h"
#include "test_channels.h"
#include "test_fs.h"
#include "test_dir.h"
#include "test_shell.h"

#define FS_STAGE_MAX    8192u        /* bounded staging buffer for a file */
#define FS_READ_CHUNK   256u         /* bytes per QT_CH_READ */
#define FS_HELLO_STATUS 7            /* hello.c: exit status on success */

volatile int r6_phase_done = 0;

static unsigned char stage[FS_STAGE_MAX];
static unsigned char head[16];

/* Sector source for the negative tests: one sector may be replaced. */
struct sector_override {
	int active;
	unsigned int lba;
	unsigned char sec[512];
};
static struct sector_override ovr;

static int ovr_read(void *ctx, unsigned int lba, unsigned char *buf)
{
	struct sector_override *o = (struct sector_override *) ctx;
	unsigned int i;
	if (o->active && lba == o->lba) {
		for (i = 0; i < 512; i++)
			buf[i] = o->sec[i];
		return 0;
	}
	return disk_read_sector(lba, buf);
}

/* Load sector lba from the disk into the override, to patch it. */
static int ovr_begin(unsigned int lba)
{
	ovr.active = 0;
	if (disk_read_sector(lba, ovr.sec) != 0)
		return 0;
	ovr.lba = lba;
	ovr.active = 1;
	return 1;
}

static void wait_job_done(int id)
{
	while (qtrap(QT_JOB_EXIT, (unsigned int) id, 0, 0) == JOB_ERR_RUNNING)
		qtrap(QT_SUSPEND, 1, 0, 0);
}

static void wait_job_reclaimed(int id)
{
	if (id <= 0)
		return;
	while (jobs[id & 0xffff].in_use)
		qtrap(QT_SUSPEND, 1, 0, 0);
}

/* Read the open channel to its end, at most FS_STAGE_MAX bytes.  Returns the
 * count, or a negative code. */
static int read_whole(int ch)
{
	unsigned int total = 0;
	for (;;) {
		unsigned int want = FS_STAGE_MAX - total;
		int n;
		if (want == 0)
			break;
		if (want > FS_READ_CHUNK)
			want = FS_READ_CHUNK;
		n = qtrap(QT_CH_READ, (unsigned int) ch, (unsigned int) (stage + total), want);
		if (n < 0)
			return n;
		if (n == 0)
			break;
		total += (unsigned int) n;
	}
	return (int) total;
}

static void negative_tests(void)
{
	struct fat16_vol nv, *win;
	struct fat16_file f, nf;
	unsigned long cnt = disk_sector_count();
	unsigned int c, off, lba;
	int rc;

	win = win1_volume();
	if (win == 0 || fat16_open_root(win, "HELLO.AOU", &f) != FAT_OK) {
		r4_add("negative tests: reference file found", 0);
		return;
	}
	c = f.first;

	/* Reference mount of the same device, without an override. */
	rc = fat16_mount(&nv, ovr_read, &ovr, cnt);
	r4_add("negative tests: reference volume mounts", rc == FAT_OK);

	/* BPB: 1024-byte sectors. */
	if (ovr_begin(0)) {
		ovr.sec[11] = 0x00;
		ovr.sec[12] = 0x04;
		rc = fat16_mount(&nv, ovr_read, &ovr, cnt);
		r4_add("negative: 1024-byte sectors refused (FAT_ERR_BPB)", rc == FAT_ERR_BPB);
		ovr.active = 0;
	}

	/* BPB: a volume of 2000 sectors has fewer than 4085 clusters: FAT12. */
	if (ovr_begin(0)) {
		ovr.sec[19] = 0xd0;
		ovr.sec[20] = 0x07;
		rc = fat16_mount(&nv, ovr_read, &ovr, cnt);
		r4_add("negative: FAT12-sized volume refused (FAT_ERR_TYPE)", rc == FAT_ERR_TYPE);
		ovr.active = 0;
	}

	/* BPB: total sectors (32768) beyond the device size. */
	if (ovr_begin(0)) {
		ovr.sec[19] = 0x00;
		ovr.sec[20] = 0x80;
		rc = fat16_mount(&nv, ovr_read, &ovr, cnt);
		r4_add("negative: volume past the device refused (FAT_ERR_BPB)", rc == FAT_ERR_BPB);
		ovr.active = 0;
	}

	/* Reference mount again, so the next cases start from a good volume. */
	rc = fat16_mount(&nv, ovr_read, &ovr, cnt);

	/* Chain: the first cluster points to itself (a cycle). */
	lba = nv.fat_start + (c * 2u) / FAT_SECTOR_BYTES;
	off = (c * 2u) % FAT_SECTOR_BYTES;
	if (ovr_begin(lba)) {
		ovr.sec[off] = (unsigned char) (c & 0xff);
		ovr.sec[off + 1] = (unsigned char) (c >> 8);
		rc = fat16_open_root(&nv, "HELLO.AOU", &nf);
		r4_add("negative: cyclic cluster chain refused (FAT_ERR_CHAIN)", rc == FAT_ERR_CHAIN);
		ovr.active = 0;
	}

	/* Chain: the first cluster is marked bad (0xfff7). */
	if (ovr_begin(lba)) {
		ovr.sec[off] = 0xf7;
		ovr.sec[off + 1] = 0xff;
		rc = fat16_open_root(&nv, "HELLO.AOU", &nf);
		r4_add("negative: bad-cluster mark in the chain refused (FAT_ERR_CHAIN)", rc == FAT_ERR_CHAIN);
		ovr.active = 0;
	}

	/* Directory: size 2 MiB, far more than the one-chain file holds. */
	if (ovr_begin((unsigned int) f.dir_lba)) {
		unsigned int d = f.dir_off;
		ovr.sec[d + 28] = 0x00;
		ovr.sec[d + 29] = 0x00;
		ovr.sec[d + 30] = 0x20;
		ovr.sec[d + 31] = 0x00;
		rc = fat16_open_root(&nv, "HELLO.AOU", &nf);
		r4_add("negative: size beyond the chain refused (FAT_ERR_SIZE)", rc == FAT_ERR_SIZE);
		ovr.active = 0;
	}

	/* Names and lookups, without any override. */
	rc = fat16_open_root(&nv, "NOSUCH.AOU", &nf);
	r4_add("negative: missing file refused (FAT_ERR_NOFILE)", rc == FAT_ERR_NOFILE);
	rc = fat16_open_root(&nv, "hello.aou", &nf);
	r4_add("negative: lowercase name refused (FAT_ERR_NAME)", rc == FAT_ERR_NAME);
	rc = fat16_open_root(&nv, "TOOLONGNAME.AOU", &nf);
	r4_add("negative: 9-character base refused (FAT_ERR_NAME)", rc == FAT_ERR_NAME);

	/* Seek past the end of the open file. */
	r4_add("negative: seek past end refused (FAT_ERR_RANGE)",
	       fat16_seek(&f, f.size + 1) == FAT_ERR_RANGE);
	fat16_seek(&f, 0);
}

void fs_phase(void)
{
	struct fat16_file ref;
	struct app_image info;
	int rc, ch, n, id, status;
	unsigned int i;
	int total;

	rc = win1_mount();
	r4_add("WIN1_ volume mounted: FAT16 boot sector and cluster count valid", rc == FAT_OK);
	if (rc != FAT_OK) {
		r6_phase_done = 1;
		return;
	}
	con_puts("WIN1_: FAT16 mounted\n");

	ch = qtrap(QT_CH_OPEN, (unsigned int) "WIN1_HELLO.AOU", 0, 0);
	r4_add("WIN1_HELLO.AOU opened through the channel manager", ch > 0);
	if (ch <= 0) {
		r6_phase_done = 1;
		return;
	}
	con_puts("WIN1_HELLO.AOU: opened\n");

	total = read_whole(ch);
	r4_add("file read in chunks through QT_CH_READ", total > 0);
	n = qtrap(QT_CH_READ, (unsigned int) ch, (unsigned int) stage, 1);
	r4_add("read at end of file returns 0", total > 0 && n == 0);
	r4_add("file bytes match the directory size",
	       fat16_open_root(win1_volume(), "HELLO.AOU", &ref) == FAT_OK &&
	       ref.size == (unsigned long) total);
	r4_add("file is a complete a.out image (R5 check accepts it)",
	       total > 0 && aout_check(stage, (unsigned int) total, &info) == APP_OK);

	/* Seek back to the start and compare the first bytes. */
	r4_add("seek to 0 accepted", qtrap(QT_CH_SEEK, (unsigned int) ch, 0, 0) == 0);
	n = qtrap(QT_CH_READ, (unsigned int) ch, (unsigned int) head, 16);
	{
		int same = (n == 16);
		for (i = 0; same && i < 16; i++)
			if (head[i] != stage[i])
				same = 0;
		r4_add("re-read after seek returns the same bytes", same);
	}
	r4_add("seek past end refused",
	       qtrap(QT_CH_SEEK, (unsigned int) ch, (unsigned int) total + 1, 0) < 0);
	r4_add("write to a file channel refused",
	       qtrap(QT_CH_WRITE, (unsigned int) ch, (unsigned int) head, 1) < 0);
	r4_add("file channel closed", qtrap(QT_CH_CLOSE, (unsigned int) ch, 0, 0) == 0);

	/* The R5 loader, from the bytes read through the file channel. */
	id = app_load_bytes(stage, (unsigned int) total);
	r4_add("HELLO.AOU loaded from the filesystem into slot 1",
	       id > 0 && app_slot_owner(1) == id);
	if (id <= 0) {
		r6_phase_done = 1;
		return;
	}
	con_puts("HELLO: loaded from filesystem\n");

	wait_job_done(id);
	status = (int) qtrap(QT_JOB_EXIT, (unsigned int) id, 0, 0);
	con_puts("HELLO: exited, status ");
	if (status >= 0)
		con_putdec((unsigned long) status);
	else
		con_puts("(none)");
	con_putc('\n');
	r4_add("HELLO.AOU exit status 7", status == FS_HELLO_STATUS);
	wait_job_reclaimed(id);
	r4_add("slot 1 released after the filesystem job", app_slot_owner(1) == 0);

	negative_tests();
	dir_phase();                    /* R7: listing, directory channel, malformed entries */
	shell_phase();                  /* R7: the shell, from the volume, into slot 0 */

	r6_phase_done = 1;
}
