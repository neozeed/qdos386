/*
 * QDOS/386 R10 - x87 support for the job scheduler.
 *
 * The scheduler is the only code that saves or restores x87 state.  The
 * kernel itself never uses floating point, so the x87 state seen by a job is
 * the state that job left, as long as every switch goes through jobs.c.
 */
#include "fpu.h"

/* The FNSAVE layout is: word 0 = control word (low 16 bits), word 1 = status
 * word (low 16 bits), word 2 = tag word (low 16 bits), words 3..6 = FIP, FCS,
 * FOO, FOS, words 7..26 = ST(0)..ST(7) as 10 bytes each.  FNINIT gives
 * CW = 0x037f, SW = 0, TW = 0xffff (all registers empty) and zero pointers. */
void fpu_clean(unsigned int *area)
{
	int i;

	for (i = 0; i < FPU_STATE_WORDS; i++)
		area[i] = 0;
	area[0] = 0x037f;
	area[2] = 0xffff;
}

void fpu_reset(void)
{
	fpu_fninit();
}

/* Checks that the FPU stores its control word and status word, and that an
 * integer loaded with FILD comes back through FISTP.  Sentinels are preloaded
 * so that a missing FPU (every x87 instruction a no-op, as with
 * [cpu] fpu = 0 on Tiny386) leaves them unchanged and fails the check. */
int fpu_selftest(void)
{
	unsigned int p[4];

	p[0] = 0xdead0000u;
	p[1] = 0xdead0000u;
	p[2] = 0;
	p[3] = 0;
	fpu_probe(p);
	if ((p[0] & 0xffffu) != 0x037fu)
		return 1;                 /* control word not stored */
	if ((p[1] & 0xffffu) != 0)
		return 2;                 /* status word not stored, or not clear */
	if (p[3] != 12345u)
		return 3;                 /* FILD / FISTP did not round-trip */
	return 0;
}
