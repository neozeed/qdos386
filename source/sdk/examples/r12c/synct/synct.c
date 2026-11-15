/* SYNCT.AOU (R12C Stage 3/4): the storage SYNC service from a C89 program.
 *   SYNCT api                    the API matrix (prints "SYNCT ok/FAIL ...", ends with "SYNCT PASS checks=N"); also
 *                                written to WIN1:/SYNCT.LOG.  Run it with the periodic sync off: it checks that the
 *                                previous interval is 0, and it leaves the periodic sync off
 *   SYNCT now                    one qd_sync(); prints "SYNCT sync rc=N" and the status counters; status 0 if rc is 0
 *   SYNCT hold FILE N T [sync]   write N lines "line k" to FILE with qd_write on a channel (no qd_flush: the data
 *                                sectors are on the device, the directory entry's size is pending until a SYNC, a
 *                                new cluster or the close; libc fflush() would call qd_flush and write it at once),
 *                                then with "sync" call qd_sync(); keep the file open T ticks; close it.
 *   SYNCT heavy FILES KB ROUNDS  ROUNDS times: write FILES files /TMP/HEAVYn.DAT of KB KiB each (a pattern that
 *                                depends on round, file and offset), read them back and compare; prints one line per
 *                                round with the number of bad bytes; status 0 when every byte matched
 * Program title: "SYNC test" (C68 _prog_name). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qdos386.h"

char _prog_name[] = "SYNC test";

static int checks, fails;
static char logbuf[4096];                /* the log is written at the end: an open log would be a pending file itself */
static unsigned int loglen;

static void check(const char *what, int ok, long got)
{
	char line[200];

	checks++;
	if (!ok)
		fails++;
	sprintf(line, "SYNCT %s %s (%ld)\n", ok ? "ok  " : "FAIL", what, got);
	if (loglen + strlen(line) < sizeof logbuf) {
		strcpy(logbuf + loglen, line);
		loglen += (unsigned int) strlen(line);
	}
	printf("%s", line);
}

static void show_status(void)
{
	unsigned long st[16];

	if (qd_sync_status(st) != 0) {
		printf("SYNCT status: not available\n");
		return;
	}
	printf("SYNCT status: syncs %lu ok %lu failed %lu last %ld files %lu deverr %lu errreg 0x%02lx periodic %lu/%lu interval %lu\n",
	       st[1], st[2], st[3], (long) (int) st[4], st[6], st[13], st[7], st[8], st[9], st[10]);
}

static void api(void)
{
	unsigned long a[16], b[16];
	FILE *f;
	int rc, i;

	rc = qd_sync_status(a);
	check("status available, version 1", rc == 0 && a[0] == 1, rc);
	rc = qd_sync();
	check("qd_sync() succeeds", rc == 0, rc);
	qd_sync_status(b);
	check("counters: syncs +1, ok +1, failed unchanged, last result 0",
	      b[1] == a[1] + 1 && b[2] == a[2] + 1 && b[3] == a[3] && b[4] == 0, (long) b[1]);
	check("a device flush was completed", b[12] == a[12] + 1, (long) b[12]);
	{
		/* raw channel writes inside the first cluster: only the size in the directory entry is pending */
		int ch = qd_open_ex("WIN1:/SYNCT.DAT", QD_O_WRITE | QD_O_CREATE | QD_O_TRUNC | QD_O_FILE);
		char line[40];

		for (i = 0; ch >= 0 && i < 20; i++) {
			sprintf(line, "SYNCT data line %d\n", i);
			qd_write_all(ch, line, (unsigned int) strlen(line));
		}
		rc = qd_sync();
		qd_sync_status(b);
		check("with a file open for writing: its directory entry is written by the sync", ch >= 0 && rc == 0 && b[6] == 1, (long) b[6]);
		rc = qd_sync();
		qd_sync_status(b);
		check("a second sync finds nothing pending", rc == 0 && b[6] == 0, (long) b[6]);
		if (ch >= 0)
			qd_close(ch);
		f = fopen("WIN1:/SYNCT.DAT", "r");
		i = 0;
		if (f != 0) {
			while (fgets(line, sizeof line, f) != 0)
				i++;
			fclose(f);
		}
		check("after the close the file reads back with all 20 lines", i == 20, (long) i);
	}
	rc = qd_sync();
	qd_sync_status(b);
	check("after the close nothing is pending (0 entries)", rc == 0 && b[6] == 0, (long) b[6]);
	rc = qd_sync_status((unsigned long *) 0x1000);
	check("status buffer in kernel memory refused -14", rc == -14, rc);
	rc = qd_sync_interval(3601);
	check("interval 3601 refused -4", rc == -4, rc);
	rc = qd_sync_interval(7);
	check("interval 7 accepted (returns the previous, 0)", rc == 0, rc);
	qd_sync_status(b);
	check("status shows interval 7", b[10] == 7, (long) b[10]);
	rc = qd_sync_interval(0);
	check("interval 0 turns it off (returns 7)", rc == 7, rc);
}

static int hold(const char *name, int n, long t, int dosync)
{
	int ch = qd_open_ex(name, QD_O_WRITE | QD_O_CREATE | QD_O_TRUNC | QD_O_FILE);
	int i, rc;
	char line[24];

	if (ch < 0) {
		printf("SYNCT hold: cannot create %s (%d)\n", name, ch);
		return 3;
	}
	for (i = 1; i <= n; i++) {
		sprintf(line, "line %d\n", i);
		if (qd_write_all(ch, line, (unsigned int) strlen(line)) < 0) {
			printf("SYNCT hold: write %d failed\n", i);
			return 4;
		}
	}
	printf("SYNCT hold: %d lines written to %s, file open\n", n, name);
	if (dosync) {
		rc = qd_sync();
		printf("SYNCT hold: sync rc=%d\n", rc);
		show_status();
	}
	printf("SYNCT hold: keeping the file open for %ld ticks\n", t);
	qd_yield((unsigned int) t);
	qd_close(ch);
	printf("SYNCT hold: closed\n");
	return 0;
}

static unsigned char pat(int round, int file, long off)
{
	return (unsigned char) ((off * 7 + file * 13 + round * 31 + (off >> 9)) & 0xff);
}

static int heavy(int files, int kb, int rounds)
{
	static unsigned char buf[1024];
	char name[32];
	int r, k, i, total = 0;
	long off;

	for (r = 0; r < rounds; r++) {
		long bad = 0;

		for (k = 0; k < files; k++) {
			FILE *f;

			sprintf(name, "WIN1:/TMP/HEAVY%d.DAT", k);
			f = fopen(name, "w");
			if (f == 0) {
				bad += 1000000L;
				continue;
			}
			for (off = 0; off < (long) kb * 1024L; off += 1024) {
				for (i = 0; i < 1024; i++)
					buf[i] = pat(r, k, off + i);
				if (fwrite(buf, 1, 1024, f) != 1024)
					bad += 1024;
			}
			if (fclose(f) != 0)
				bad += 1;
		}
		for (k = 0; k < files; k++) {
			FILE *f;

			sprintf(name, "WIN1:/TMP/HEAVY%d.DAT", k);
			f = fopen(name, "r");
			if (f == 0) {
				bad += 1000000L;
				continue;
			}
			for (off = 0; off < (long) kb * 1024L; off += 1024) {
				if (fread(buf, 1, 1024, f) != 1024) {
					bad += 1024;
					break;
				}
				for (i = 0; i < 1024; i++)
					if (buf[i] != pat(r, k, off + i))
						bad++;
			}
			fclose(f);
		}
		printf("SYNCT heavy round %d: %d files x %d KiB, bad bytes %ld\n", r + 1, files, kb, bad);
		if (bad)
			total++;
	}
	show_status();
	printf("SYNCT heavy %s\n", total == 0 ? "PASS" : "FAIL");
	return total == 0 ? 0 : 5;
}

int main(int argc, char **argv)
{
	if (argc > 1 && strcmp(argv[1], "now") == 0) {
		int rc = qd_sync();

		printf("SYNCT sync rc=%d\n", rc);
		show_status();
		return rc == 0 ? 0 : 1;
	}
	if (argc > 4 && strcmp(argv[1], "hold") == 0)
		return hold(argv[2], atoi(argv[3]), atol(argv[4]), argc > 5 && strcmp(argv[5], "sync") == 0);
	if (argc > 4 && strcmp(argv[1], "heavy") == 0)
		return heavy(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
	{
		int logch;
		char l[64];

		api();
		sprintf(l, "SYNCT %s checks=%d fails=%d\n", fails == 0 ? "PASS" : "FAIL", checks, fails);
		printf("%s", l);
		logch = qd_open_ex("WIN1:/SYNCT.LOG", QD_O_WRITE | QD_O_CREATE | QD_O_TRUNC | QD_O_FILE);
		if (logch >= 0) {
			qd_write_all(logch, logbuf, loglen);
			qd_write_all(logch, l, (unsigned int) strlen(l));
			qd_close(logch);
		}
	}
	return fails == 0 ? 0 : 3;
}
