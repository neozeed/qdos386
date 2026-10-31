/* HEAPT.AOU (R12A Gate 2): private heap, private stack, x87 state and read-only FAT use under preemption.
 *   usage: HEAPT tag rounds status
 * Every round allocates, resizes or frees one of 24 blocks (malloc/realloc/free = the process's private heap) filled
 * with a pattern made from the tag, so a block written by another copy at the same address would be seen.  Every 16th
 * round recurses 40 levels with 512-byte frames on the private stack; every 50th reads LONG.TXT and compares its sum;
 * every 64th validates the heap.  An x87 accumulator is checked against a replay at the end.  Between yields the loop
 * is CPU bound, so the timer preempts it on its user stack.  Prints HEAPT <tag> PASS ... and ends with status. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "qdos386.h"

#define NB 24
static unsigned char *blk[NB];
static unsigned int sz[NB];
static unsigned long seed;
static int tagc;

static unsigned int rnd(void)
{
	seed = seed * 1103515245ul + 12345ul;
	return (unsigned int) (seed >> 8) & 0xFFFFFFu;
}

static unsigned char pat(int k, unsigned int i)
{
	return (unsigned char) (tagc * 7 + k * 13 + (int) i);
}

static int check_blk(int k, unsigned int n)
{
	unsigned int i;

	for (i = 0; i < n; i++)
		if (blk[k][i] != pat(k, i))
			return 1;
	return 0;
}

static void fill_blk(int k)
{
	unsigned int i;

	for (i = 0; i < sz[k]; i++)
		blk[k][i] = pat(k, i);
}

static int deep(int n, unsigned int salt)
{
	volatile unsigned char pad[512];
	int i, r = 0;

	for (i = 0; i < 512; i++)
		pad[i] = (unsigned char) (salt + (unsigned int) i + (unsigned int) n);
	if (n > 0)
		r = deep(n - 1, salt);
	for (i = 0; i < 512; i++)
		if (pad[i] != (unsigned char) (salt + (unsigned int) i + (unsigned int) n))
			r++;
	return r;
}

static long file_sum(void)
{
	int ch = qd_open("WIN1_LONG.TXT"), n, i;
	unsigned char b[256];
	long s = 0;

	if (ch < 0)
		return -1;
	while ((n = qd_read(ch, b, sizeof b)) > 0)
		for (i = 0; i < n; i++)
			s = (s * 31 + b[i]) & 0x3FFFFFFFl;          /* stays positive; -1 means "could not open" */
	qd_close(ch);
	return s;
}

int main(int argc, char **argv)
{
	struct qd_procinfo pi;
	unsigned int r, rounds, i, maxbrk = 0, spin;
	int k, status, bad = 0, stackbad = 0, filebad = 0, heapbad = 0, nomem = 0;
	long fs0;
	double acc = 0.0, replay = 0.0;

	if (argc < 4) {
		printf("usage: HEAPT tag rounds status\n");
		return 1;
	}
	tagc = argv[1][0];
	seed = (unsigned long) tagc * 2654435761ul;
	rounds = (unsigned int) atoi(argv[2]);
	status = atoi(argv[3]);
	fs0 = file_sum();
	for (r = 0; r < rounds; r++) {
		k = (int) (rnd() % NB);
		if (blk[k] == 0) {
			sz[k] = 1u + rnd() % ((rnd() & 3u) == 0 ? 60000u : 2000u);
			blk[k] = (unsigned char *) malloc(sz[k]);
			if (blk[k] == 0) {
				nomem++;
				continue;
			}
			for (i = 0; i < sz[k]; i++)
				if (blk[k][i] != 0)
					bad++;
			fill_blk(k);
		} else if (rnd() & 1u) {
			unsigned int nsz = 1u + rnd() % 30000u, keep = nsz < sz[k] ? nsz : sz[k];
			unsigned char *q;
			bad += check_blk(k, sz[k]);
			q = (unsigned char *) realloc(blk[k], nsz);
			if (q == 0) {
				nomem++;
				continue;
			}
			blk[k] = q;
			bad += check_blk(k, keep);
			sz[k] = nsz;
			fill_blk(k);
		} else {
			bad += check_blk(k, sz[k]);
			free(blk[k]);
			blk[k] = 0;
		}
		acc = acc * 0.999 + sqrt((double) (k + 1));
		if ((r & 15u) == 15u)
			stackbad += deep(40, (unsigned int) tagc + r);
		if ((r % 50u) == 49u && file_sum() != fs0)
			filebad++;
		if ((r & 63u) == 63u && qd_mem_validate() != 0)
			heapbad++;
		if (qd_proc_info(0, &pi) == 0 && pi.heap_brk - pi.heap_base > maxbrk)
			maxbrk = (unsigned int) (pi.heap_brk - pi.heap_base);
		for (spin = 0; spin < 20000u; spin++)
			;
		if ((r & 7u) == 7u)
			qd_yield(1);
	}
	for (k = 0; k < NB; k++)
		if (blk[k] != 0) {
			bad += check_blk(k, sz[k]);
			free(blk[k]);
			blk[k] = 0;
		}
	seed = (unsigned long) tagc * 2654435761ul;          /* replay the accumulator alone */
	{
		unsigned int rr;
		int kk;
		unsigned char *dummy[NB];
		for (kk = 0; kk < NB; kk++)
			dummy[kk] = 0;
		for (rr = 0; rr < rounds; rr++) {
			kk = (int) (rnd() % NB);
			if (dummy[kk] == 0) {
				rnd();
				rnd();
				dummy[kk] = (unsigned char *) 1;
			} else if (rnd() & 1u) {
				rnd();
			} else {
				dummy[kk] = 0;
			}
			replay = replay * 0.999 + sqrt((double) (kk + 1));
		}
	}
	qd_proc_info(0, &pi);
	printf("HEAPT %c stats r=%u heap=%u end=%lu fr=%lu pre=%lu khwm=%lu\n", tagc, rounds, maxbrk, pi.heap_used,
	       pi.frames, pi.preempts, pi.kstack_hwm);
	if (bad == 0 && stackbad == 0 && filebad == 0 && heapbad == 0 && acc == replay && fs0 != -1 && nomem == 0) {
		printf("HEAPT %c PASS exit %d\n", tagc, status);
		return status;
	}
	printf("HEAPT %c FAIL bad=%d stack=%d file=%d heap=%d nomem=%d fp=%s\n", tagc, bad, stackbad, filebad, heapbad,
	       nomem, acc == replay ? "ok" : "DIFF");
	return 99;
}
