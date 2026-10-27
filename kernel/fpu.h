/*
 * QDOS/386 R10 - per-job x87 state.
 *
 * Each job keeps an FNSAVE image (108 bytes, 27 words) of its x87 state:
 * control word, status word, tag word, instruction and operand pointers and
 * the eight register slots.  The scheduler saves the outgoing job's image
 * and restores the incoming one whenever it switches jobs (kernel/jobs.c).
 */
#ifndef QDOS386_FPU_H
#define QDOS386_FPU_H

#define FPU_STATE_WORDS 27

void fpu_fninit(void);                 /* FNINIT: clean FPU, control word 0x037f */
void fpu_save(unsigned int *area);     /* FNSAVE: store the state, then reinitialise */
void fpu_restore(unsigned int *area);  /* FRSTOR: load a saved state */
void fpu_probe(unsigned int *out);     /* see context.s */

void fpu_clean(unsigned int *area);    /* the state FNINIT gives, as an FNSAVE image */
void fpu_reset(void);                  /* FNINIT at boot */
int fpu_selftest(void);                /* 0 = FPU works; else a failure code */

#endif
