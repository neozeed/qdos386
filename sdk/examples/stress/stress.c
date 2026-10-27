/*
 * SDK example: stress.  Allocates and frees blocks of several sizes, keeps a
 * table of live blocks, fills and checks each one, checks the heap after
 * every round, and finishes with every block released and the free count
 * restored.  Exit 0 on success.  Prints one line.
 * Build:  qdos386-gcc -o SDKSTS.aout stress.c
 */
#include "qdos386.h"

#define LIVE    24
#define ROUNDS  40

static unsigned char *live[LIVE];
static unsigned int live_n[LIVE];

static unsigned int size_for(unsigned int round, unsigned int slot)
{
	return 16u + ((round * 37u + slot * 101u) % 1500u);
}

static int fill_check(unsigned char *p, unsigned int n, unsigned int seed, int do_fill)
{
	unsigned int i;
	for (i = 0; i < n; i++) {
		unsigned char want = (unsigned char) ((i * 7u + seed) & 255u);
		if (do_fill)
			p[i] = want;
		else if (p[i] != want)
			return 0;
	}
	return 1;
}

int main(int argc, char **argv)
{
	unsigned int round, slot, free0, n;
	int con = qd_open("CON");
	int failures = 0;

	(void) argc;
	(void) argv;
	free0 = qd_free_bytes();
	for (round = 0; round < ROUNDS; round++) {
		for (slot = 0; slot < LIVE; slot++) {
			if (live[slot] != 0) {
				if (!fill_check(live[slot], live_n[slot], slot, 0))
					failures++;
				if (qd_free(live[slot]) != 0)
					failures++;
				live[slot] = 0;
			}
			n = size_for(round, slot);
			live[slot] = (unsigned char *) qd_alloc(n);
			if (live[slot] == 0) {
				failures++;
				continue;
			}
			live_n[slot] = n;
			fill_check(live[slot], n, slot, 1);
		}
		if (!qd_heap_ok())
			failures++;
		qd_yield(1);
	}
	for (slot = 0; slot < LIVE; slot++) {
		if (live[slot] != 0) {
			if (!fill_check(live[slot], live_n[slot], slot, 0))
				failures++;
			if (qd_free(live[slot]) != 0)
				failures++;
			live[slot] = 0;
		}
	}
	if (qd_free_bytes() != free0)
		failures++;
	if (failures == 0)
		qd_write(con, "QDOS/386 SDK STRESS PASS\n", 25);
	else
		qd_write(con, "QDOS/386 SDK STRESS FAIL\n", 25);
	qd_close(con);
	return failures == 0 ? 0 : 1;
}
