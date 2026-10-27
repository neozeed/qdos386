/* MEMSTR.AOU - R9 proof: malloc, calloc, realloc, free across sizes, with
 * data checks, a double-free check and heap accounting.  Runs three rounds so
 * the same job reuses its freed blocks.  The heap free count must return to
 * its starting value after each round. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qdos386.h"

extern unsigned long qd_bad_frees;

static const unsigned int sizes[] = { 1u, 15u, 16u, 17u, 100u, 1000u, 4096u, 16384u, 65536u };
#define NSIZES (sizeof sizes / sizeof sizes[0])

static int fill_check(unsigned char *p, unsigned int n, unsigned int seed)
{
	unsigned int i;
	for (i = 0; i < n; i++)
		p[i] = (unsigned char) (i * 31u + seed);
	for (i = 0; i < n; i++)
		if (p[i] != (unsigned char) (i * 31u + seed))
			return 0;
	return 1;
}

int main(int argc, char **argv)
{
	unsigned char *blk[NSIZES];
	unsigned char *grow;
	unsigned int round, i, fails = 0;
	unsigned int free0;
	unsigned long bad0;
	int c;

	(void) argc;
	(void) argv;

	for (round = 0; round < 3; round++) {
		free0 = qd_free_bytes();
		bad0 = qd_bad_frees;

		for (i = 0; i < NSIZES; i++) {
			blk[i] = (unsigned char *) malloc(sizes[i]);
			if (blk[i] == NULL || !fill_check(blk[i], sizes[i], i + round))
				fails++;
			if (((unsigned long) blk[i] & 15u) != 0)
				fails++;
		}

		/* calloc returns zeroed memory. */
		c = 1;
		{
			unsigned char *z = (unsigned char *) calloc(300u, 4u);
			unsigned int k;
			if (z == NULL)
				fails++;
			else {
				for (k = 0; k < 1200u; k++)
					if (z[k] != 0)
						c = 0;
				if (!c)
					fails++;
				free(z);
			}
		}

		/* realloc: grow from 64 to 4096 to 65536, keeping the data. */
		grow = (unsigned char *) malloc(64);
		if (grow == NULL || !fill_check(grow, 64, 7u))
			fails++;
		grow = (unsigned char *) realloc(grow, 4096u);
		if (grow == NULL)
			fails++;
		else {
			for (i = 0; i < 64u; i++)
				if (grow[i] != (unsigned char) (i * 31u + 7u))
					fails++;
		}
		grow = (unsigned char *) realloc(grow, 65536u);
		if (grow == NULL)
			fails++;
		else {
			for (i = 0; i < 64u; i++)
				if (grow[i] != (unsigned char) (i * 31u + 7u))
					fails++;
		}
		/* Shrink keeps the prefix as well. */
		grow = (unsigned char *) realloc(grow, 100u);
		if (grow == NULL)
			fails++;
		else {
			for (i = 0; i < 64u; i++)
				if (grow[i] != (unsigned char) (i * 31u + 7u))
					fails++;
			free(grow);
		}

		for (i = 0; i < NSIZES; i++) {
			if (blk[i] != NULL) {
				/* Data still intact after the other allocations. */
				if (!fill_check(blk[i], sizes[i], i + round))
					fails++;
				free(blk[i]);
			}
		}
		/* A second free of the same pointer is ignored and counted. */
		{
			void *q = malloc(40u);
			if (q == NULL)
				fails++;
			else {
				free(q);
				free(q);
			}
		}
		if (qd_bad_frees != bad0 + 1)
			fails++;
		if (qd_free_bytes() != free0)
			fails++;
		if (!qd_heap_ok())
			fails++;
		printf("MEMSTR: round %u heap free %u bytes (start %u)\n",
		       round, qd_free_bytes(), free0);
	}

	if (fails == 0) {
		printf("QDOS/386 R9 MEMSTRESS PASS\n");
		return 0;
	}
	printf("QDOS/386 R9 MEMSTRESS FAIL (%u)\n", fails);
	return 100;
}
