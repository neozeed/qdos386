/*
 * QDOS/386 R7 - root directory checks.
 *
 * The volume (tools/make-fat16.py) holds, in directory order: a label, a
 * deleted entry, a long-name fragment, the eight regular files, a
 * subdirectory, an invalid name, a CHAIN.AOU whose first cluster is out of
 * range, an end-of-directory marker, and TRAILER.TXT after the marker.  The
 * listing must show exactly the eight regular files, in order.  Every check
 * is recorded with r4_add and printed by IDLE.
 */
#include "platform.h"
#include "jobs.h"
#include "channel.h"
#include "fat16.h"
#include "win1.h"
#include "test_channels.h"
#include "test_dir.h"

#define DIR_EXPECT 10
#define DIR_MAX    32
#define DIR_REC    16u

static const char *expect[DIR_EXPECT] = {
	"HELLO.AOU", "PREP.AOU", "SLOT0.AOU", "README.TXT",
	"BIG.AOU", "BAD.AOU", "EMPTY.TXT", "SHELL.AOU", "SHELLT.AOU", "CHAIN.AOU"
};

/* A 12-byte name field (padded with NULs) equals a C string. */
static int name12_eq(const char *got, const char *want)
{
	unsigned int i;
	for (i = 0; i < 12; i++) {
		if (want[i] == 0) {
			for (; i < 12; i++)
				if (got[i] != 0)
					return 0;
			return 1;
		}
		if (got[i] != want[i])
			return 0;
	}
	return want[12] == 0;
}

/* The same comparison on a 16-byte record copied out of a channel buffer. */
static int rec_name_eq(const unsigned char *rec, const char *want)
{
	return name12_eq((const char *) rec, want);
}

static unsigned long rec_size(const unsigned char *rec)
{
	return (unsigned long) rec[12] | ((unsigned long) rec[13] << 8) |
	       ((unsigned long) rec[14] << 16) | ((unsigned long) rec[15] << 24);
}

static struct fat16_dirent listed[DIR_MAX];

void dir_phase(void)
{
	struct fat16_vol *v = win1_volume();
	struct fat16_file dh, ff;
	struct fat16_dirent d;
	unsigned int n = 0, i;
	int rc, ok, ch;
	unsigned char *buf;
	unsigned int got;
	unsigned long skipped0;
	int names_ok = 1, sizes_ok = 1;

	if (v == 0) {
		r4_add("root listing: volume mounted", 0);
		return;
	}

	/* 1. The listing: exactly the eight regular files, in directory order. */
	fat16_dir_skipped = 0;
	skipped0 = fat16_dir_skipped;
	fat16_dir_open(v, &dh);
	for (;;) {
		rc = fat16_dir_next(&dh, &d);
		if (rc <= 0)
			break;
		if (n < DIR_MAX)
			listed[n] = d;
		n++;
	}
	r4_add("root listing: end reached without error", rc == 0);
	r4_add("root listing: exactly the ten regular files", n == DIR_EXPECT);
	for (i = 0; i < n && i < DIR_EXPECT; i++)
		if (!name12_eq(listed[i].name, expect[i]))
			names_ok = 0;
	r4_add("root listing: names in directory order", names_ok && n == DIR_EXPECT);

	/* 2. Sizes come from the directory entries. */
	for (i = 0; i < n && i < DIR_EXPECT; i++)
		if (listed[i].size == 0 && i != 6)      /* EMPTY.TXT is the only empty file */
			sizes_ok = 0;
	ok = n == DIR_EXPECT && listed[4].size == 8200 && listed[6].size == 0;
	r4_add("root listing: sizes BIG.AOU 8200, EMPTY.TXT 0", ok && sizes_ok);

	/* 3. The invalid name was skipped and counted; the rest were not counted. */
	r4_add("root listing: invalid name skipped and counted once",
	       fat16_dir_skipped - skipped0 == 1);

	/* 4. Names that must not open. */
	r4_add("CHAIN.AOU: listed, but its chain is refused at open",
	       fat16_open_root(v, "CHAIN.AOU", &ff) == FAT_ERR_CHAIN);
	r4_add("TRAILER.TXT after the end marker: not opened",
	       fat16_open_root(v, "TRAILER.TXT", &ff) == FAT_ERR_NOFILE);
	r4_add("GONE.TXT deleted entry: not opened",
	       fat16_open_root(v, "GONE.TXT", &ff) == FAT_ERR_NOFILE);
	r4_add("SUBDIR subdirectory: not opened as a file",
	       fat16_open_root(v, "SUBDIR", &ff) == FAT_ERR_NOFILE);

	/* 5. An empty file opens and reads nothing. */
	rc = fat16_open_root(v, "EMPTY.TXT", &ff);
	if (rc == FAT_OK) {
		unsigned char one[4];
		unsigned int g = 99;
		int rr = fat16_read(&ff, one, sizeof one, &g);
		r4_add("EMPTY.TXT: opens with size 0, reads 0 bytes",
		       ff.size == 0 && rr == FAT_OK && g == 0);
	} else {
		r4_add("EMPTY.TXT: opens with size 0, reads 0 bytes", 0);
	}

	/* 6. The directory channel: 16-byte records through WIN1_. */
	buf = (unsigned char *) qtrap(QT_MEM_ALLOC, 256, 0, 0);
	ch = (int) qtrap(QT_CH_OPEN, (unsigned int) "WIN1_", 0, 0);
	if (buf == 0 || ch < 0) {
		r4_add("directory channel: opens as WIN1_", 0);
		if (ch >= 0)
			qtrap(QT_CH_CLOSE, (unsigned int) ch, 0, 0);
		return;
	}
	r4_add("directory channel: opens as WIN1_", ch > 0);

	rc = (int) qtrap(QT_CH_READ, (unsigned int) ch, (unsigned int) buf, 8);
	r4_add("directory channel: a buffer under one record is refused",
	       rc == CHAN_ERR_RANGE);

	got = (unsigned int) qtrap(QT_CH_READ, (unsigned int) ch, (unsigned int) buf, 256);
	r4_add("directory channel: ten records, 160 bytes", got == DIR_EXPECT * DIR_REC);
	ok = got == DIR_EXPECT * DIR_REC &&
	     rec_name_eq(buf, "HELLO.AOU") &&
	     rec_name_eq(buf + 9 * DIR_REC, "CHAIN.AOU") &&
	     rec_size(buf + 9 * DIR_REC) == 100 &&
	     rec_size(buf + 4 * DIR_REC) == 8200;
	r4_add("directory channel: first record HELLO.AOU, CHAIN.AOU size 100, BIG.AOU 8200", ok);

	rc = (int) qtrap(QT_CH_READ, (unsigned int) ch, (unsigned int) buf, 256);
	r4_add("directory channel: end of listing reads 0", rc == 0);

	r4_add("directory channel: seek to 0 rewinds", qtrap(QT_CH_SEEK, (unsigned int) ch, 0, 0) == 0);
	got = (unsigned int) qtrap(QT_CH_READ, (unsigned int) ch, (unsigned int) buf, 256);
	r4_add("directory channel: listing repeats after rewind",
	       got == DIR_EXPECT * DIR_REC && rec_name_eq(buf, "HELLO.AOU"));
	r4_add("directory channel: seek past 0 refused",
	       qtrap(QT_CH_SEEK, (unsigned int) ch, 5, 0) < 0);

	qtrap(QT_CH_CLOSE, (unsigned int) ch, 0, 0);
	qtrap(QT_MEM_FREE, (unsigned int) buf, 0, 0);
}
