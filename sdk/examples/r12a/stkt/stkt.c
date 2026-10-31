/* STKT.AOU (R12A): recursion on the private user stack.
 *   usage: STKT depth tag      each level uses a 1 KiB frame; the user stack is 64 KiB with an unmapped guard below.
 * Prints "STKT <tag> depth <d> ok" at every 8 levels on the way down; "STKT <tag> PASS depth <d>" if it comes back.
 * A depth past the stack runs into the guard: the kernel ends only this process (#DF task gate) and reports it. */
#include <stdio.h>
#include <stdlib.h>
#include "qdos386.h"

static int tagc;

static int down(int d, int max)
{
	volatile unsigned char frame[1024];
	int i, r = 0;

	for (i = 0; i < 1024; i++)
		frame[i] = (unsigned char) (d + i);
	if ((d & 7) == 0)
		printf("STKT %c depth %d ok\n", tagc, d);
	if (d < max)
		r = down(d + 1, max);
	for (i = 0; i < 1024; i++)
		if (frame[i] != (unsigned char) (d + i))
			r++;
	return r;
}

int main(int argc, char **argv)
{
	int depth = argc > 1 ? atoi(argv[1]) : 32, bad;

	tagc = argc > 2 ? argv[2][0] : 'S';
	printf("STKT %c start depth %d\n", tagc, depth);
	bad = down(1, depth);
	printf("STKT %c %s depth %d\n", tagc, bad == 0 ? "PASS" : "FAIL", depth);
	return bad == 0 ? 0 : 3;
}
