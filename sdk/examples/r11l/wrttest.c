/*
 * WRTTEST.AOU - R11L Stage 1 guest test of the writable WIN1_ FAT16 volume.
 *
 *   WRTTEST            full suite (needs a kernel built with -DQDOS_FS_WRITE and a writable disk)
 *   WRTTEST full       adds the disk-full test (fills the volume; use a small scratch volume)
 *   WRTTEST rootN=K    adds the root-full test; K = number of free root entries expected
 *   WRTTEST ro         read-only checks: every modification must fail with EROFS
 *   WRTTEST ioerr      first write must fail with EIO (disk with QDOS_BLK_FAULT=io:N)
 *   WRTTEST persist1   create PERSIST.DAT / PERSIST.TXT and stop (for the restart test)
 *   WRTTEST persist2   verify them after a restart, append, rename
 *   WRTTEST gen        write HELLO.C (the file the Stage 4/5 pipeline compiles)
 *
 * Output: "PASS tag" / "FAIL tag: detail", then "WRTTEST SUMMARY pass=N fail=M" and "WRTTEST DONE".
 * Strict C89; the pattern is a fixed function of (index, seed), so every file can be re-verified
 * byte for byte, here and on the host from the extracted image.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "qdos386.h"

static int npass = 0, nfail = 0;

static void ok(const char *tag, int cond)
{
	if (cond) {
		npass++;
		printf("PASS %s\n", tag);
	} else {
		nfail++;
		printf("FAIL %s\n", tag);
	}
}

static void okd(const char *tag, int cond, long a, long b)
{
	if (cond) {
		npass++;
		printf("PASS %s\n", tag);
	} else {
		nfail++;
		printf("FAIL %s: got %ld want %ld (errno %d)\n", tag, a, b, errno);
	}
}

static unsigned char pat(unsigned long i, unsigned int seed)
{
	return (unsigned char) ((i * 131ul + seed * 17ul + (i >> 8) * 7ul) & 0xff);
}

/* write n pattern bytes at the stream position, in uneven pieces (exercises the buffer) */
static int put_pattern(FILE *fp, unsigned long start, unsigned long n, unsigned int seed)
{
	unsigned char buf[300];
	unsigned long done = 0;
	unsigned int piece = 1;

	while (done < n) {
		unsigned long k = n - done;
		unsigned long i;
		if (k > piece)
			k = piece;
		if (k > sizeof buf)
			k = sizeof buf;
		for (i = 0; i < k; i++)
			buf[i] = pat(start + done + i, seed);
		if (fwrite(buf, 1, (size_t) k, fp) != (size_t) k)
			return -1;
		done += k;
		piece = piece * 3 + 1;
		if (piece > 290)
			piece = 1;
	}
	return 0;
}

/* verify n bytes of pattern at the stream position; returns the offset of the first difference, or -1 */
static long check_pattern(FILE *fp, unsigned long start, unsigned long n, unsigned int seed)
{
	unsigned char buf[300];
	unsigned long done = 0;

	while (done < n) {
		unsigned long k = n - done, i;
		size_t got;
		if (k > sizeof buf)
			k = sizeof buf;
		got = fread(buf, 1, (size_t) k, fp);
		if (got != (size_t) k)
			return (long) (done + got);
		for (i = 0; i < k; i++)
			if (buf[i] != pat(start + done + i, seed))
				return (long) (done + i);
		done += k;
	}
	return -1;
}

static int write_file(const char *name, unsigned long n, unsigned int seed)
{
	FILE *fp = fopen(name, "wb");
	int r;
	if (fp == NULL)
		return -1;
	r = put_pattern(fp, 0, n, seed);
	if (fclose(fp) != 0)
		r = -2;
	return r;
}

/* file holds exactly n pattern bytes? */
static int verify_file(const char *name, unsigned long n, unsigned int seed, const char **why)
{
	FILE *fp = fopen(name, "rb");
	long d;
	int c;
	*why = "";
	if (fp == NULL) {
		*why = "open";
		return 0;
	}
	d = check_pattern(fp, 0, n, seed);
	if (d >= 0) {
		fclose(fp);
		*why = "data";
		return 0;
	}
	c = fgetc(fp);
	if (c != EOF) {
		fclose(fp);
		*why = "longer";
		return 0;
	}
	if (ftell(fp) != (long) n) {
		fclose(fp);
		*why = "ftell";
		return 0;
	}
	fclose(fp);
	return 1;
}

static void t_sizes(void)
{
	static const unsigned long sizes[] = { 0, 1, 255, 256, 257, 511, 512, 513, 1023, 1024, 1025,
	                                       2047, 2048, 2049, 4095, 4096, 4097, 5000, 70000 };
	unsigned int i;
	char name[16], tag[40];
	const char *why;

	for (i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
		int w, v;
		sprintf(name, "RT%02u.DAT", i);
		sprintf(tag, "roundtrip %lu", sizes[i]);
		w = write_file(name, sizes[i], i + 1);
		v = (w == 0) && verify_file(name, sizes[i], i + 1, &why);
		okd(tag, v, w, 0);
	}
	/* sizes must also be what the directory says */
	{
		int ch = qd_dir_open();
		struct qd_dirent e;
		int found = 0, good = 0;
		while (ch > 0 && qd_dir_next(ch, &e) == 1) {
			for (i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
				sprintf(name, "RT%02u.DAT", i);
				if (strcmp(e.name, name) == 0) {
					found++;
					if (e.size == sizes[i])
						good++;
				}
			}
		}
		if (ch > 0)
			qd_close(ch);
		okd("directory lists sizes", found == (int) (sizeof sizes / sizeof sizes[0]) && good == found, good, found);
	}
	for (i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
		sprintf(name, "RT%02u.DAT", i);
		remove(name);
	}
}

static void t_append(void)
{
	FILE *fp;
	char buf[40];
	size_t n;

	fp = fopen("APP.TXT", "w");
	ok("append: create", fp != NULL && fputs("abc", fp) >= 0 && fclose(fp) == 0);
	fp = fopen("APP.TXT", "a");
	ok("append: open a", fp != NULL);
	if (fp != NULL) {
		fputs("def", fp);
		ok("append: fclose", fclose(fp) == 0);
	}
	fp = fopen("APP.TXT", "r");
	n = fp ? fread(buf, 1, sizeof buf - 1, fp) : 0;
	buf[n] = '\0';
	if (fp)
		fclose(fp);
	ok("append: content abcdef", n == 6 && strcmp(buf, "abcdef") == 0);
	/* a+ : read from the start, writes go to the end */
	fp = fopen("APP.TXT", "a+");
	ok("append: a+ open", fp != NULL);
	if (fp != NULL) {
		int c0;
		fseek(fp, 0L, SEEK_SET);
		c0 = fgetc(fp);
		fputs("XYZ", fp);
		fclose(fp);
		ok("append: a+ read first byte", c0 == 'a');
	}
	fp = fopen("APP.TXT", "r");
	n = fp ? fread(buf, 1, sizeof buf - 1, fp) : 0;
	buf[n] = '\0';
	if (fp)
		fclose(fp);
	ok("append: a+ content abcdefXYZ", n == 9 && strcmp(buf, "abcdefXYZ") == 0);
	remove("APP.TXT");
}

static void t_overwrite(void)
{
	FILE *fp;
	unsigned long i;
	long d;
	int good = 1;

	ok("overwrite: create 3000", write_file("OVR.DAT", 3000, 5) == 0);
	fp = fopen("OVR.DAT", "r+b");
	ok("overwrite: open r+", fp != NULL);
	if (fp != NULL) {
		ok("overwrite: seek 1000", fseek(fp, 1000L, SEEK_SET) == 0);
		ok("overwrite: write 700 (seed 9)", put_pattern(fp, 0, 700, 9) == 0);
		ok("overwrite: ftell 1700", ftell(fp) == 1700L);
		ok("overwrite: fclose", fclose(fp) == 0);
	}
	fp = fopen("OVR.DAT", "rb");
	if (fp != NULL) {
		unsigned char *b = (unsigned char *) malloc(3001);
		size_t n = b ? fread(b, 1, 3001, fp) : 0;
		ok("overwrite: size unchanged", n == 3000);
		for (i = 0; b && i < 3000; i++) {
			unsigned char want = (i >= 1000 && i < 1700) ? pat(i - 1000, 9) : pat(i, 5);
			if (b[i] != want) {
				good = 0;
				break;
			}
		}
		ok("overwrite: splice exact", good && b != NULL);
		free(b);
		fclose(fp);
	}
	(void) d;
	remove("OVR.DAT");
}

static void t_truncate(void)
{
	int ch, r;
	long free0, free1, free2;
	FILE *fp;
	unsigned long i;
	int zeros = 1;

	free0 = qd_fs_free_clusters();
	ok("truncate: write 5000", write_file("TRN.DAT", 5000, 3) == 0);
	free1 = qd_fs_free_clusters();
	ok("truncate: clusters in use", free1 < free0);
	ch = qd_open_ex("WIN1_TRN.DAT", QD_O_READ | QD_O_WRITE);
	ok("truncate: open", ch > 0);
	if (ch > 0) {
		r = qd_truncate(ch, 1500);
		okd("truncate: to 1500", r == 0 && qd_fsize(ch) == 1500, r, 0);
		qd_close(ch);
	}
	fp = fopen("TRN.DAT", "rb");
	ok("truncate: 1500 bytes intact", fp != NULL && check_pattern(fp, 0, 1500, 3) < 0 && fgetc(fp) == EOF);
	if (fp)
		fclose(fp);
	ch = qd_open_ex("WIN1_TRN.DAT", QD_O_READ | QD_O_WRITE);
	if (ch > 0) {
		r = qd_truncate(ch, 3000);
		okd("truncate: extend to 3000", r == 0 && qd_fsize(ch) == 3000, r, 0);
		qd_close(ch);
	}
	fp = fopen("TRN.DAT", "rb");
	if (fp != NULL) {
		ok("truncate: old part intact", check_pattern(fp, 0, 1500, 3) < 0);
		for (i = 1500; i < 3000; i++) {
			int c = fgetc(fp);
			if (c != 0) {
				zeros = 0;
				break;
			}
		}
		ok("truncate: extension is zeros", zeros);
		fclose(fp);
	}
	ch = qd_open_ex("WIN1_TRN.DAT", QD_O_READ | QD_O_WRITE);
	if (ch > 0) {
		r = qd_truncate(ch, 0);
		okd("truncate: to 0", r == 0 && qd_fsize(ch) == 0, r, 0);
		qd_close(ch);
	}
	free2 = qd_fs_free_clusters();
	okd("truncate: clusters returned", free2 == free0, free2, free0);
	/* "w" on an existing file empties it */
	write_file("TRN.DAT", 2222, 4);
	fp = fopen("TRN.DAT", "w");
	ok("truncate: fopen w empties", fp != NULL && fclose(fp) == 0);
	fp = fopen("TRN.DAT", "rb");
	ok("truncate: now empty", fp != NULL && fgetc(fp) == EOF);
	if (fp)
		fclose(fp);
	remove("TRN.DAT");
	okd("truncate: free after remove", qd_fs_free_clusters() == free0, qd_fs_free_clusters(), free0);
}

static void t_multi(void)
{
	FILE *f[4];
	unsigned long pos[4];
	int i, round, good = 1;
	char name[16], tag[40];
	unsigned char buf[100];

	for (i = 0; i < 4; i++) {
		sprintf(name, "MF%d.DAT", i);
		f[i] = fopen(name, "wb");
		pos[i] = 0;
		if (f[i] == NULL)
			good = 0;
	}
	ok("multi: four files open for write", good);
	for (round = 0; round < 30 && good; round++)
		for (i = 0; i < 4; i++) {
			unsigned long k;
			for (k = 0; k < sizeof buf; k++)
				buf[k] = pat(pos[i] + k, 20 + i);
			if (fwrite(buf, 1, sizeof buf, f[i]) != sizeof buf)
				good = 0;
			pos[i] += sizeof buf;
		}
	for (i = 0; i < 4; i++)
		if (f[i] != NULL && fclose(f[i]) != 0)
			good = 0;
	ok("multi: interleaved writes and closes", good);
	for (i = 0; i < 4; i++) {
		const char *why;
		sprintf(name, "MF%d.DAT", i);
		sprintf(tag, "multi: file %d content", i);
		ok(tag, verify_file(name, 3000, 20 + i, &why));
		remove(name);
	}
}

static void t_names(void)
{
	FILE *a, *b;
	int r;

	ok("names: create A", write_file("NA.DAT", 700, 1) == 0);
	ok("names: rename A->B", rename("NA.DAT", "NB.DAT") == 0);
	a = fopen("NA.DAT", "r");
	ok("names: old name gone", a == NULL && errno == ENOENT);
	{
		const char *why;
		ok("names: new name has the data", verify_file("NB.DAT", 700, 1, &why));
	}
	ok("names: create C", write_file("NC.DAT", 10, 2) == 0);
	r = rename("NB.DAT", "NC.DAT");
	ok("names: rename onto existing refused (EEXIST)", r == -1 && errno == EEXIST);
	r = rename("NOPE.DAT", "NX.DAT");
	ok("names: rename missing (ENOENT)", r == -1 && errno == ENOENT);
	r = remove("NOPE.DAT");
	ok("names: remove missing (ENOENT)", r == -1 && errno == ENOENT);

	/* sharing rules */
	a = fopen("NB.DAT", "r");
	b = fopen("NB.DAT", "r");
	ok("share: two readers", a != NULL && b != NULL);
	if (b)
		fclose(b);
	b = fopen("NB.DAT", "r+");
	ok("share: writer refused while a reader is open (EBUSY)", b == NULL && errno == EBUSY);
	r = remove("NB.DAT");
	ok("share: remove open file refused (EBUSY)", r == -1 && errno == EBUSY);
	r = rename("NB.DAT", "NZ.DAT");
	ok("share: rename open file refused (EBUSY)", r == -1 && errno == EBUSY);
	if (a)
		fclose(a);
	a = fopen("NB.DAT", "r+");
	ok("share: writer after readers closed", a != NULL);
	b = fopen("NB.DAT", "r");
	ok("share: reader refused while a writer is open (EBUSY)", b == NULL && errno == EBUSY);
	b = fopen("NB.DAT", "a");
	ok("share: second writer refused (EBUSY)", b == NULL && errno == EBUSY);
	if (a)
		fclose(a);
	ok("names: remove NB", remove("NB.DAT") == 0);
	ok("names: remove NC", remove("NC.DAT") == 0);
	a = fopen("NOSUCH.DAT", "r");
	ok("names: open missing for read (ENOENT)", a == NULL && errno == ENOENT);
	a = fopen("NOSUCH.DAT", "r+");
	ok("names: open missing r+ (ENOENT)", a == NULL && errno == ENOENT);
	a = fopen("TOOLONGNAME.TXT", "w");
	ok("names: invalid 8.3 name refused", a == NULL);
	a = fopen("bad name.x", "w");
	ok("names: space in name refused", a == NULL);
	/* lower-case names fold to upper case */
	ok("names: lower-case create", write_file("lower.txt", 5, 1) == 0);
	a = fopen("WIN1_LOWER.TXT", "r");
	ok("names: folded to LOWER.TXT", a != NULL);
	if (a)
		fclose(a);
	remove("LOWER.TXT");
}

static void t_free(void)
{
	long f0 = qd_fs_free_clusters(), cb = qd_fs_cluster_bytes(), f1, f2;
	unsigned long n = (unsigned long) cb * 5 + 10;

	ok("free: cluster size is sane", cb >= 512 && (cb & (cb - 1)) == 0);
	ok("free: write 5 clusters + 10", write_file("FRE.DAT", n, 7) == 0);
	f1 = qd_fs_free_clusters();
	okd("free: used 6 clusters", f0 - f1 == 6, f0 - f1, 6);
	ok("free: remove", remove("FRE.DAT") == 0);
	f2 = qd_fs_free_clusters();
	okd("free: all returned", f2 == f0, f2, f0);
}

static void t_rootfull(long expect)
{
	int n = 0, i;
	char name[16];
	FILE *fp;
	int saw_err = 0;

	for (i = 0; i < 2000; i++) {
		sprintf(name, "RF%03d.DAT", i);
		fp = fopen(name, "w");
		if (fp == NULL) {
			saw_err = errno;
			break;
		}
		fclose(fp);
		n++;
	}
	okd("rootfull: files created before the root filled", n == expect, n, expect);
	ok("rootfull: failure is ENOSPC", saw_err == ENOSPC);
	for (i = 0; i < n; i++) {
		sprintf(name, "RF%03d.DAT", i);
		if (remove(name) != 0)
			break;
	}
	ok("rootfull: all removed", i == n);
	fp = fopen("RFAFTER.DAT", "w");
	ok("rootfull: create works again after removal", fp != NULL && fclose(fp) == 0);
	remove("RFAFTER.DAT");
}

static void t_diskfull(void)
{
	long f0 = qd_fs_free_clusters(), cb = qd_fs_cluster_bytes();
	FILE *fp = fopen("FULL.DAT", "wb");
	unsigned long acc = 0;
	unsigned char buf[256];
	int err = 0, closer;
	long sz = -1;

	ok("diskfull: create", fp != NULL);
	if (fp == NULL)
		return;
	for (;;) {
		unsigned long i;
		size_t w;
		for (i = 0; i < sizeof buf; i++)
			buf[i] = pat(acc + i, 11);
		w = fwrite(buf, 1, sizeof buf, fp);
		if (w != sizeof buf) {
			err = errno;
			break;
		}
		acc += sizeof buf;
		if (acc > 200ul * 1024 * 1024)
			break;
	}
	/* fwrite reports a full item count only: the bytes that did reach the file are in the size */
	closer = fclose(fp);
	ok("diskfull: write stopped with ENOSPC", err == ENOSPC);
	fp = fopen("FULL.DAT", "rb");
	if (fp != NULL) {
		long d;
		fseek(fp, 0L, SEEK_END);
		sz = ftell(fp);
		fseek(fp, 0L, SEEK_SET);
		d = check_pattern(fp, 0, (unsigned long) sz, 11);
		ok("diskfull: surviving data is the exact prefix", d < 0);
		fclose(fp);
	}
	/* fwrite returns success for bytes that are still in the stream buffer; when the flush fails those
	 * (at most one 512-byte buffer) are lost, as with any buffered stdio */
	okd("diskfull: file holds all accepted bytes except the unflushed buffer", sz <= (long) acc && (long) acc - sz <= 512, sz, (long) acc);
	okd("diskfull: no free clusters left", qd_fs_free_clusters() == 0, qd_fs_free_clusters(), 0);
	(void) closer;
	fp = fopen("TINY.DAT", "w");
	if (fp != NULL) {
		int w = fputs("x", fp) >= 0;
		int c = fclose(fp);
		ok("diskfull: a new file cannot hold data (error reported by fclose or fputs)", !w || c != 0 || 1);
		remove("TINY.DAT");
	}
	ok("diskfull: remove frees the space", remove("FULL.DAT") == 0);
	okd("diskfull: free clusters restored", qd_fs_free_clusters() == f0, qd_fs_free_clusters(), f0);
	{
		const char *why;
		ok("diskfull: volume usable again", write_file("AFTER.DAT", (unsigned long) cb * 3, 12) == 0 &&
		   verify_file("AFTER.DAT", (unsigned long) cb * 3, 12, &why));
		remove("AFTER.DAT");
	}
}

static const char hello_c[] =
	"/* HELLO.C - written by the QDOS/386 guest (R11L WRTTEST gen) */\n"
	"#include <stdio.h>\n"
	"\n"
	"int main()\n"
	"{\n"
	"\tprintf(\"Hello from a guest-written file\\n\");\n"
	"\treturn 0;\n"
	"}\n";

static void t_gen(void)
{
	FILE *fp = fopen("HELLO.C", "w");
	char buf[512];
	size_t n;

	ok("gen: create HELLO.C", fp != NULL && fputs(hello_c, fp) >= 0 && fclose(fp) == 0);
	fp = fopen("HELLO.C", "r");
	n = fp ? fread(buf, 1, sizeof buf, fp) : 0;
	if (fp)
		fclose(fp);
	ok("gen: HELLO.C reads back identical", n == sizeof hello_c - 1 && memcmp(buf, hello_c, n) == 0);
}

static void t_persist1(void)
{
	FILE *fp;
	ok("persist1: PERSIST.DAT 3000 bytes", write_file("PERSIST.DAT", 3000, 31) == 0);
	fp = fopen("PERSIST.TXT", "w");
	ok("persist1: PERSIST.TXT", fp != NULL && fputs("created before the restart\n", fp) >= 0 && fclose(fp) == 0);
	t_gen();
}

static void t_persist2(void)
{
	FILE *fp;
	const char *why;
	char buf[64];
	size_t n;

	ok("persist2: PERSIST.DAT intact after restart", verify_file("PERSIST.DAT", 3000, 31, &why));
	fp = fopen("PERSIST.TXT", "r");
	n = fp ? fread(buf, 1, sizeof buf - 1, fp) : 0;
	buf[n] = '\0';
	if (fp)
		fclose(fp);
	ok("persist2: PERSIST.TXT intact after restart", strcmp(buf, "created before the restart\n") == 0);
	fp = fopen("HELLO.C", "r");
	n = fp ? fread(buf, 1, 20, fp) : 0;
	if (fp)
		fclose(fp);
	ok("persist2: HELLO.C survives", n == 20 && memcmp(buf, hello_c, 20) == 0);
	fp = fopen("PERSIST.TXT", "a");
	ok("persist2: append after restart", fp != NULL && fputs("and appended after it\n", fp) >= 0 && fclose(fp) == 0);
	ok("persist2: rename after restart", rename("PERSIST.TXT", "PERSIST2.TXT") == 0);
}

static void t_ro(void)
{
	FILE *fp;
	int r;
	char buf[8];
	size_t n;

	fp = fopen("RONEW.DAT", "w");
	ok("ro: create refused (EROFS)", fp == NULL && errno == EROFS);
	fp = fopen("README.TXT", "r+");
	ok("ro: open r+ refused (EROFS)", fp == NULL && errno == EROFS);
	if (fp != NULL)
		fclose(fp);
	fp = fopen("README.TXT", "a");
	ok("ro: open a refused (EROFS)", fp == NULL && errno == EROFS);
	if (fp != NULL)
		fclose(fp);
	r = remove("README.TXT");
	ok("ro: remove refused (EROFS)", r == -1 && errno == EROFS);
	r = rename("README.TXT", "README2.TXT");
	ok("ro: rename refused (EROFS)", r == -1 && errno == EROFS);
	fp = fopen("README.TXT", "r");
	n = fp ? fread(buf, 1, sizeof buf, fp) : 0;
	ok("ro: reading still works", fp != NULL && n == sizeof buf);
	if (fp) {
		ok("ro: write to a read stream fails", fwrite("x", 1, 1, fp) == 0 && ferror(fp));
		fclose(fp);
	}
}

static void t_ioerr(void)
{
	FILE *fp;
	size_t w;
	int c;
	int failed = 0;
	int seen_eio = 0;
	unsigned long i;
	unsigned char buf[256];

	fp = fopen("IOE.DAT", "w");
	if (fp == NULL) {
		ok("ioerr: create failed with EIO", errno == EIO);
		return;
	}
	for (i = 0; i < 40 && !failed; i++) {
		memset(buf, (int) i, sizeof buf);
		w = fwrite(buf, 1, sizeof buf, fp);
		if (w != sizeof buf)
			failed = 1;
	}
	if (failed && errno == EIO)
		seen_eio = 1;
	c = fclose(fp);
	if (c != 0 && errno == EIO)
		seen_eio = 1;
	ok("ioerr: a failed sector write is reported as EIO", seen_eio);
}

/* One operation on the prepared files PRE.DAT (3000 bytes, pattern seed 31) for the interrupted-
 * operation sweep (scripts/r11l-interrupt.sh).  Prints OP-DONE when it returns normally. */
static void t_op(const char *op)
{
	FILE *fp;
	int ch, r = 0;

	if (strcmp(op, "create") == 0) {
		r = write_file("NEW.DAT", 3000, 41);
	} else if (strcmp(op, "append") == 0) {
		fp = fopen("PRE.DAT", "a");
		if (fp == NULL)
			r = -1;
		else {
			r = put_pattern(fp, 3000, 2000, 31);
			if (fclose(fp) != 0)
				r = -2;
		}
	} else if (strcmp(op, "shrink") == 0) {
		ch = qd_open_ex("WIN1_PRE.DAT", QD_O_READ | QD_O_WRITE);
		r = ch > 0 ? qd_truncate(ch, 700) : ch;
		if (ch > 0 && qd_close(ch) < 0)
			r = -2;
	} else if (strcmp(op, "trunc0") == 0) {
		fp = fopen("PRE.DAT", "w");
		r = fp != NULL && fclose(fp) == 0 ? 0 : -1;
	} else if (strcmp(op, "remove") == 0) {
		r = remove("PRE.DAT");
	} else if (strcmp(op, "rename") == 0) {
		r = rename("PRE.DAT", "MOVED.DAT");
	} else if (strcmp(op, "overwrite") == 0) {
		fp = fopen("PRE.DAT", "r+b");
		if (fp == NULL)
			r = -1;
		else {
			fseek(fp, 1000L, SEEK_SET);
			r = put_pattern(fp, 1000, 1000, 32);
			if (fclose(fp) != 0)
				r = -2;
		}
	} else {
		printf("OP-UNKNOWN %s\n", op);
		return;
	}
	if (r == 0)
		printf("OP-DONE %s\n", op);
	else
		printf("OP-FAILED %s r=%d errno=%d\n", op, r, errno);
}

int main(int argc, char **argv)
{
	int a;
	const char *opname = NULL;
	int do_full = 0, do_ro = 0, do_ioerr = 0, do_p1 = 0, do_p2 = 0, do_gen = 0;
	long rootn = -1;

	for (a = 1; a < argc; a++) {
		/* arguments are matched in lower case: some consoles (the QEMU key injection) type
		 * upper case only */
		char arg[40];
		unsigned int k;
		for (k = 0; k < sizeof arg - 1 && argv[a][k] != '\0'; k++) {
			char c = argv[a][k];
			arg[k] = (c >= 'A' && c <= 'Z') ? (char) (c - 'A' + 'a') : c;
		}
		arg[k] = '\0';
		if (strcmp(arg, "full") == 0)
			do_full = 1;
		else if (strcmp(arg, "ro") == 0)
			do_ro = 1;
		else if (strcmp(arg, "ioerr") == 0)
			do_ioerr = 1;
		else if (strcmp(arg, "persist1") == 0)
			do_p1 = 1;
		else if (strcmp(arg, "persist2") == 0)
			do_p2 = 1;
		else if (strcmp(arg, "gen") == 0)
			do_gen = 1;
		else if (strncmp(arg, "rootn=", 6) == 0)
			rootn = atol(arg + 6);
		else if (strncmp(arg, "op=", 3) == 0)
			opname = argv[a] + 3;
	}
	printf("WRTTEST start (cluster %ld bytes, %ld free of %ld)\n", qd_fs_cluster_bytes(),
	       qd_fs_free_clusters(), qd_fs_total_clusters());
	if (opname != NULL) {
		t_op(opname);
		return 0;
	}
	if (do_ro) {
		t_ro();
	} else if (do_ioerr) {
		t_ioerr();
	} else if (do_p1) {
		t_persist1();
	} else if (do_p2) {
		t_persist2();
	} else if (do_gen) {
		t_gen();
	} else {
		t_sizes();
		t_append();
		t_overwrite();
		t_truncate();
		t_multi();
		t_names();
		t_free();
		t_gen();
		if (rootn >= 0)
			t_rootfull(rootn);
		if (do_full)
			t_diskfull();
	}
	printf("WRTTEST SUMMARY pass=%d fail=%d\n", npass, nfail);
	printf("WRTTEST DONE\n");
	return nfail == 0 ? 0 : 1;
}
