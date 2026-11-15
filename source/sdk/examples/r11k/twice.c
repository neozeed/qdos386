/* R11K TWICE - an ordinary C89 program, nothing in it knows about paging.  Built with the
 * unchanged SDK (qdos386-gcc -slot 1 -O); one file, linked once at 0x240000.  Two (or three) copies
 * of the very same file are started by PGRUN (or by the TWICET acceptance driver) in separate
 * address spaces and run at the same time.
 *
 *   TWICE [ticks [tag [file hexsum]]]
 *     ticks  how long to keep running (default 40 timer ticks)
 *     tag    one character 0..9, folded into the exit status (default 0)
 *     file   optional FAT16 root file to read through stdio; hexsum = its expected checksum
 *
 * What each copy checks, with expectations derived only from its own tagged job id (qd_self),
 * which is never in the window: if two copies shared a frame, each would find the other's values.
 *   - initialised data has the compiled-in values; BSS is zero (the loader clears it)
 *   - it then overwrites both with job-specific patterns and re-verifies them throughout, across
 *     timer preemptions and cooperative yields (a CR3 switch each time while both are live)
 *   - a stdio line buffer, malloc blocks (shared kernel heap, owner-reclaimed), memcpy/qsort/sprintf
 *   - an x87 accumulator that must be exact at the end (the FPU state follows the job)
 *   - optionally a file checksum read in 100-byte fread chunks
 * Exit status: 0x40 + tag on success; 1..31 = the first check that failed. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qdos386.h"

#define NI 32
#define NB 128

static unsigned long init_tab[NI] = {
	0x11111111ul, 0x22222222ul, 0x33333333ul, 0x44444444ul, 0x55555555ul, 0x66666666ul, 0x77777777ul, 0x88888888ul,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24
};
static char init_str[24] = "TWICE-initialised-data";
static unsigned long bss_tab[NB];
static char line[96];
static unsigned long counter;

static int failed;
/* The identity comes from the kernel every time, never from a variable in the window. */
#define sid (qd_self())

static void fail(int code, const char *what)
{
	if (!failed) {
		failed = code;
		printf("TWICE[%x]: CHECK %d FAILED: %s\n", sid & 0xffff, code, what);
	}
}

static unsigned long pat(int i)
{
	return ((unsigned long) sid * 40503ul) ^ ((unsigned long) i * 2654435761ul);
}

static int cmp_ul(const void *a, const void *b)
{
	unsigned long x = *(const unsigned long *) a, y = *(const unsigned long *) b;
	return x < y ? -1 : (x > y ? 1 : 0);
}

static void verify(void)
{
	int i;

	for (i = 0; i < NI; i++)
		if (init_tab[i] != (pat(i) ^ 0x5A5A5A5Aul))
			fail(3, "initialised data overwritten");
	for (i = 0; i < NB; i++)
		if (bss_tab[i] != pat(i + 1000))
			fail(4, "BSS overwritten");
	if (line[0] != (char) ('A' + (sid & 15)) || strncmp(line + 1, "TWICE tag ", 10) != 0)
		fail(5, "line buffer overwritten");
	if (counter != (((unsigned long) sid << 24) & 0xff000000ul))
		fail(6, "counter overwritten");
}

static unsigned long file_sum(const char *name, unsigned long *len)
{
	FILE *fp = fopen(name, "r");
	unsigned char buf[100];
	unsigned long s = 0, n = 0;
	size_t got, k;

	*len = 0;
	if (fp == NULL)
		return 0xFFFFFFFFul;
	while ((got = fread(buf, 1, sizeof buf, fp)) != 0) {
		for (k = 0; k < got; k++) {
			s = s * 31ul + buf[k];
			n++;
		}
		if ((n % 4096ul) < sizeof buf)
			verify();
	}
	fclose(fp);
	*len = n;
	return s;
}

static unsigned long hexv(const char *s)
{
	unsigned long v = 0;

	for (; *s; s++) {
		v <<= 4;
		if (*s >= '0' && *s <= '9') v |= (unsigned long) (*s - '0');
		else if (*s >= 'a' && *s <= 'f') v |= (unsigned long) (*s - 'a' + 10);
		else if (*s >= 'A' && *s <= 'F') v |= (unsigned long) (*s - 'A' + 10);
	}
	return v;
}

int main(int argc, char **argv)
{
	unsigned long t0, t, last, loops = 0, a = 1;
	unsigned long ticks = 40;
	int tag = 0, i, round;
	double x = 0.0, step;
	unsigned long *blk, sorted[64];
	char *argcopy;

	if (argc >= 2)
		ticks = (unsigned long) atoi(argv[1]);
	if (argc >= 3 && argv[2][0] >= '0' && argv[2][0] <= '9')
		tag = argv[2][0] - '0';
	argcopy = (char *) malloc(strlen(argv[0]) + 1);
	if (argcopy == NULL) { fail(7, "malloc"); return 7; }
	strcpy(argcopy, argv[0]);

	/* 1. the image arrived intact */
	for (i = 0; i < NI; i++)
		if (init_tab[i] == 0)
			fail(1, "initialised data zero");
	if (init_tab[0] != 0x11111111ul || init_tab[7] != 0x88888888ul || init_tab[31] != 24 ||
	    strcmp(init_str, "TWICE-initialised-data") != 0)
		fail(1, "initialised data wrong");
	for (i = 0; i < NB; i++)
		if (bss_tab[i] != 0)
			fail(2, "BSS not zero");
	if (counter != 0)
		fail(2, "counter not zero");

	/* 2. job-specific state in the same virtual addresses */
	for (i = 0; i < NI; i++)
		init_tab[i] = pat(i) ^ 0x5A5A5A5Aul;
	for (i = 0; i < NB; i++)
		bss_tab[i] = pat(i + 1000);
	sprintf(line, "%cTWICE tag %d", 'A' + (sid & 15), tag);
	counter = ((unsigned long) sid << 24) & 0xff000000ul;
	step = 0.25 * (double) (tag + 1);
	printf("TWICE[%x] tag %d start: argv0 %s tab %08x bss %08x line %08x tick %u\n", sid & 0xffff, tag,
	       argcopy, (unsigned int) init_tab, (unsigned int) bss_tab, (unsigned int) line,
	       (unsigned int) qd_ticks());

	/* 3. compute, switched by the timer; yield now and then */
	t0 = qd_ticks();
	last = t0;
	do {
		for (i = 0; i < 1500; i++) {
			a = a * 1664525ul + 1013904223ul;
			x += step;
			loops++;
		}
		verify();
		for (round = 0; round < 64; round++)
			sorted[round] = (a * (unsigned long) (round + 7)) ^ pat(round);
		qsort(sorted, 64, sizeof sorted[0], cmp_ul);
		for (round = 1; round < 64; round++)
			if (sorted[round - 1] > sorted[round])
				fail(8, "qsort");
		blk = (unsigned long *) malloc(256);
		if (blk == NULL) { fail(9, "malloc"); break; }
		for (round = 0; round < 64; round++)
			blk[round] = pat(round + 5000);
		if ((loops & 0x3FFF) == 0)
			qd_yield(1);          /* cooperative switch */
		for (round = 0; round < 64; round++)
			if (blk[round] != pat(round + 5000))
				fail(10, "heap block");
		free(blk);
		t = qd_ticks();
		if (t - last >= 10u) {
			printf("TWICE[%x] tag %d tick %u loops %u\n", sid & 0xffff, tag, (unsigned int) t,
			       (unsigned int) loops);
			last = t;
		}
	} while (t - t0 < ticks && !failed);
	verify();
	if (x != step * (double) loops)
		fail(11, "x87 accumulator");
	if (strcmp(argcopy, argv[0]) != 0)
		fail(12, "argv copy");

	/* 4. optional FAT16 read through stdio */
	if (argc >= 5) {
		unsigned long len, sum = file_sum(argv[3], &len), want = hexv(argv[4]);
		printf("TWICE[%x] tag %d file %s: %u bytes, checksum %08x (expected %08x)\n", sid & 0xffff, tag,
		       argv[3], (unsigned int) len, (unsigned int) sum, (unsigned int) want);
		if (sum != want)
			fail(13, "file checksum");
		verify();
	}
	free(argcopy);
	if (failed) {
		printf("TWICE[%x] tag %d: FAIL %d\n", sid & 0xffff, tag, failed);
		return failed;
	}
	printf("TWICE[%x] tag %d: OK loops %u\n", sid & 0xffff, tag, (unsigned int) loops);
	return 0x40 + tag;
}
