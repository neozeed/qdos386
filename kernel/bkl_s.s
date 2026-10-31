/* QDOS/386 R16 - BKL primitives (out of line: a compiler barrier for GCC 1.40). */
.text
.globl _bkl_xchg
_bkl_xchg:
	movl 4(%esp),%edx
	movl 8(%esp),%eax
	xchgl %eax,(%edx)		/* xchg with memory is implicitly locked */
	ret
/* bkl_cmpxchg(ptr, expected, newval) -> old value.  Writes newval into *ptr ONLY if *ptr == expected, atomically.
 * A spinlock whose word carries the owner id MUST use this, not xchg: an xchg writes unconditionally, so a CPU that
 * loses the race still clobbers the winner's owner id, and the winner can then never release it (two CPUs deadlock).
 * GAS 1.38 has no cmpxchg mnemonic: hand-encode LOCK CMPXCHG %ecx,(%edx) = F0 0F B1 0A (expected in EAX). */
.globl _bkl_cmpxchg
_bkl_cmpxchg:
	movl 4(%esp),%edx
	movl 8(%esp),%eax
	movl 12(%esp),%ecx
	.byte 0xF0,0x0F,0xB1,0x0A
	ret
.globl _bkl_pause
_bkl_pause:
	.byte 0xf3,0x90			/* pause */
	ret
