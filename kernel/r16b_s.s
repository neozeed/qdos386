/* QDOS/386 R16B - small helpers linked only into the opt-in R16B kernel (qdos386-r16b-pc). */
.text
/* unsigned int r16b_tsc(void): low 32 bits of the time-stamp counter.  Used only for differences (BKL wait time),
 * so wrap-around is harmless for waits shorter than 2^32 cycles.  GAS 1.38 has no rdtsc mnemonic: 0F 31. */
.globl _r16b_tsc
_r16b_tsc:
	.byte 0x0F,0x31
	ret
