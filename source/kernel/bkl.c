/* QDOS/386 R16 - BKL state and the recursive, owner-checked acquire/release (kernel/bkl.h). */
#include "bkl.h"
volatile unsigned int bkl_word = 0;
#ifdef QDOS_R17
int bkl_depth[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned long bkl_acq[8] = {0,0,0,0,0,0,0,0}, bkl_rel[8] = {0,0,0,0,0,0,0,0}, bkl_spins[8] = {0,0,0,0,0,0,0,0};
unsigned long bkl_maxspin = 0, bkl_badrel = 0;
unsigned long bkl_wait_kc[8] = {0,0,0,0,0,0,0,0}, bkl_contended[8] = {0,0,0,0,0,0,0,0};
unsigned int r16b_tsc(void);                  /* r16b_s.s */
#else
int bkl_depth[2] = { 0, 0 };
unsigned long bkl_acq[2] = {0,0}, bkl_rel[2] = {0,0}, bkl_spins[2] = {0,0}, bkl_maxspin = 0, bkl_badrel = 0;
#endif
#if defined(QDOS_R16B) && !defined(QDOS_R17)
/* R16B: time spent waiting for the lock, in units of 1024 TSC cycles, per CPU; and contended acquisitions. */
unsigned long bkl_wait_kc[2] = {0,0}, bkl_contended[2] = {0,0};
unsigned int r16b_tsc(void);                  /* r16b_s.s */
#endif

void bkl_enter(void)
{
	int c = kx_this_cpu();
	unsigned long n = 0;
#ifdef QDOS_R16B
	unsigned int t0;
#endif
	if (bkl_word == (unsigned int) (c + 1)) {   /* already ours: recurse */
		bkl_depth[c]++;
		return;
	}
#ifdef QDOS_R16B
	if (bkl_cmpxchg(&bkl_word, 0u, (unsigned int) (c + 1)) == 0u)
		goto got;                               /* uncontended: no timing overhead */
	t0 = r16b_tsc();
	bkl_contended[c]++;
#endif
	/* Acquire with compare-and-swap, NOT xchg: write our owner id only when the word is 0, so a CPU that loses the
	 * race never clobbers the winner's id (which would wedge both CPUs - the winner could never release). */
	while (bkl_cmpxchg(&bkl_word, 0u, (unsigned int) (c + 1)) != 0u) {
		bkl_pause();
		n++;
	}
#ifdef QDOS_R16B
	bkl_wait_kc[c] += (unsigned long) ((r16b_tsc() - t0) >> 10);
got:
#endif
	bkl_depth[c] = 1;
	bkl_acq[c]++;
	bkl_spins[c] += n;
	if (n > bkl_maxspin)
		bkl_maxspin = n;
}

void bkl_leave(void)
{
	int c = kx_this_cpu();
	if (bkl_word != (unsigned int) (c + 1)) {   /* not ours: nothing to do (e.g. a user-mode entry path) */
		bkl_badrel++;
		return;
	}
	if (--bkl_depth[c] > 0)
		return;
	bkl_rel[c]++;
	(void) bkl_xchg(&bkl_word, 0u);
}
