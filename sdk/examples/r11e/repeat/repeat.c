/* REPEAT: one program run many times from the shell.  Each round opens the
 * fixture, reads it through stdio, closes it, and allocates and frees heap.
 * The heap and the channel use must return to their starting values. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../check.h"
#include "qdos386.h"

#define ROUNDS 40

int main(void)
{
	unsigned int free0;
	unsigned int free1;
	int round;
	int bad;
	char buf[64];
	FILE *f;
	int lines;
	void *blocks[8];
	int i;

	/* The console channel is opened by the first printf and is kept for the
	 * life of the job, so open it before the baseline is taken. */
	printf("REPEAT: start\n");
	free0 = qd_free_bytes();
	bad = 0;
	for (round = 0; round < ROUNDS; round++) {
		lines = 0;
		f = fopen("LINES.TXT", "r");
		if (f == NULL) {
			bad = 1;
			break;
		}
		while (fgets(buf, sizeof buf, f) != NULL)
			lines++;
		if (lines != 4 || !feof(f))
			bad = 1;
		fclose(f);
		for (i = 0; i < 8; i++) {
			blocks[i] = malloc(100 + i * 37);
			if (blocks[i] == NULL)
				bad = 1;
		}
		for (i = 7; i >= 0; i--)
			free(blocks[i]);
		if (round % 10 == 0)
			printf("REPEAT round %d: free heap %u bytes\n", round, qd_free_bytes());
	}
	free1 = qd_free_bytes();
	CHECK("every round read 4 lines", bad == 0);
	CHECK("heap returns to start", free1 == free0);
	CHECK("heap invariants hold", qd_heap_ok() == 1);
	CHECK("channel still opens after repeats", qd_close(qd_open("WIN1_LINES.TXT")) >= 0);
	/* R11E: repeated libqdos use.  A RUN program cannot start child jobs (one
	 * child slot), so this loop cycles the job, channel and heap wrappers instead.
	 * Each cycle must return the heap to its start. */
	{
		int k, ok = 1;
		unsigned int c0 = qd_free_bytes();
		unsigned int inc, acc;
		for (k = 0; k < 40; k++) {
			int ch = qd_open("WIN1_LINES.TXT");
			void *blk = 0;
			if (ch <= 0 || qd_getc(ch) != 'a' || qd_close(ch) != 0)
				ok = 0;
			if (qd_alloc_ex(100 + k, &blk) != 0 || qd_free(blk) != 0)
				ok = 0;
			if (qd_job_info(qd_self(), &inc, &acc) != 0)
				ok = 0;
		}
		CHECK("40 channel, heap and job cycles succeed", ok);
		CHECK("cycles leave the heap unchanged", qd_free_bytes() == c0);
		CHECK("heap invariants after cycles", qd_heap_ok() == 1);
	}
	return finish("REPEAT");
}
