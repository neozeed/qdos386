/*
 * QDOS/386 R15 AMP0 - CPU 0 (BSP) primitives for kernel/amp.c.  Separate from the worker's own copies in amp_ap_s.s, so
 * that neither side's objects reference the other's code (tests/r15/check-ap-objects.py).  Each shared-memory access
 * is an out-of-line call: a compiler barrier for GCC 1.40.
 */
	.text
	.globl _amp_has_cpuid
_amp_has_cpuid:				/* 1 if EFLAGS.ID can be toggled */
	pushfl
	popl %eax
	movl %eax,%ecx
	xorl $0x00200000,%eax
	pushl %eax
	popfl
	pushfl
	popl %eax
	pushl %ecx
	popfl
	xorl %ecx,%eax
	shrl $21,%eax
	andl $1,%eax
	ret
	.globl _amp_cpuid
_amp_cpuid:				/* void amp_cpuid(unsigned leaf, unsigned r[4]): eax ebx ecx edx */
	pushl %ebx
	pushl %edi
	movl 12(%esp),%eax
	movl 16(%esp),%edi
	xorl %ecx,%ecx
	.byte 0x0F,0xA2			/* cpuid */
	movl %eax,(%edi)
	movl %ebx,4(%edi)
	movl %ecx,8(%edi)
	movl %edx,12(%edi)
	popl %edi
	popl %ebx
	ret
	.globl _amp_rdmsr
_amp_rdmsr:				/* unsigned amp_rdmsr(unsigned msr, unsigned *hi) -> low word */
	movl 4(%esp),%ecx
	.byte 0x0F,0x32			/* rdmsr */
	movl 8(%esp),%ecx
	movl %edx,(%ecx)
	ret
	.globl _amp_load
_amp_load:
	movl 4(%esp),%eax
	movl (%eax),%eax
	ret
	.globl _amp_xchg
_amp_xchg:				/* locked exchange, returns the old value */
	movl 4(%esp),%edx
	movl 8(%esp),%eax
	xchgl %eax,(%edx)
	ret
	.globl _amp_cas
_amp_cas:				/* lock cmpxchg; returns the value found (== old: swapped) */
	movl 4(%esp),%edx
	movl 8(%esp),%eax
	movl 12(%esp),%ecx
	.byte 0xF0,0x0F,0xB1,0x0A	/* lock cmpxchgl %ecx,(%edx) */
	ret
	.globl _amp_pause
_amp_pause:
	.byte 0xF3,0x90
	ret
	.globl _amp_mmio_rd
_amp_mmio_rd:
	movl 4(%esp),%eax
	movl (%eax),%eax
	ret
	.globl _amp_mmio_wr
_amp_mmio_wr:
	movl 4(%esp),%edx
	movl 8(%esp),%eax
	movl %eax,(%edx)
	ret
