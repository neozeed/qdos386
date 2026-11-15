/* HEAPEX.AOU - R9 proof: allocator exhaustion, cleanup and reclaim at exit.
 *
 *   HEAPEX           allocate 64 KiB blocks until malloc returns NULL (between
 *                    8 and 63 blocks: the heap size differs by target), check
 *                    the NULL is clean, free everything, check the heap count
 *                    and heap_ok, and allocate one block again.  It prints its
 *                    starting free count; the session compares that count with
 *                    the one from a later run after a leak (the heap size is
 *                    platform dependent, so no fixed number is checked here).
 *   HEAPEX leak      allocate 100000 bytes and exit without freeing.  The kernel
 *                    reclaims the job's blocks at exit, so the next plain run
 *                    must still find the heap whole.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qdos386.h"

#define MAXB 64

int main(int argc, char **argv)
{
	void *b[MAXB];
	unsigned int n = 0, i, free0, fails = 0;
	void *p;

	if (argc == 2 && strcmp(argv[1], "leak") == 0) {
		p = malloc(100000u);
		if (p == NULL)
			return 90;
		memset(p, 0x5a, 100000u);
		printf("HEAPEX: leaked 100000 bytes on purpose; exiting without free\n");
		return 0;
	}

	/* The first printf opens the CON channel, which takes heap bytes for the
	 * job.  Open it before the reference count, so the count is taken with
	 * the channel already held. */
	printf("HEAPEX: start\n");
	free0 = qd_free_bytes();
	printf("HEAPEX: start free %u\n", free0);

	while (n < MAXB) {
		p = malloc(65536u);
		if (p == NULL)
			break;
		memset(p, (int) n, 65536u);
		b[n++] = p;
	}
	if (n < 8 || n == MAXB)
		fails++;                       /* must run out, after several blocks */
	printf("HEAPEX: %u blocks of 64 KiB, then NULL\n", n);
	if (malloc(1000000u) != NULL)
		fails++;                       /* a block that cannot fit is refused */

	for (i = 0; i < n; i++) {
		unsigned char *q = (unsigned char *) b[i];
		if (q[0] != (unsigned char) i || q[65535] != (unsigned char) i)
			fails++;                   /* each block kept its own data */
		free(b[i]);
	}
	if (qd_free_bytes() != free0)
		fails++;
	if (!qd_heap_ok())
		fails++;

	p = malloc(65536u);
	if (p == NULL)
		fails++;
	free(p);
	printf("HEAPEX: heap restored to %u free after exhaustion and cleanup\n", qd_free_bytes());

	if (fails == 0) {
		printf("QDOS/386 R9 HEAPEX PASS\n");
		return 0;
	}
	printf("QDOS/386 R9 HEAPEX FAIL (%u)\n", fails);
	return 100 + fails;
}
