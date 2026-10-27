/*
 * qtrap.s - the C-callable trap entry (libqdos).
 *   int qtrap(unsigned fn, unsigned a, unsigned b, unsigned c)
 * Loads EAX = fn, EBX = a, ECX = b, EDX = c, executes int $0x30 and returns
 * EAX.  EBX is saved because it is callee-saved in the GCC 1.40 convention.
 */
.text
.globl _qtrap
_qtrap:
	pushl %ebx
	movl 8(%esp),%eax
	movl 12(%esp),%ebx
	movl 16(%esp),%ecx
	movl 20(%esp),%edx
	int $0x30
	popl %ebx
	ret

/* R11E: the entry frame of the job, recorded by crt0 at _start.  qd_end_job
 * restores it and returns from _start, which ends the job exactly as main's
 * return does; exit() therefore needs no return from main. */
.data
.globl _qd_entry_frame
_qd_entry_frame:
	.long 0
.text
.globl _qd_end_job
_qd_end_job:
	movl _qd_entry_frame,%eax
	testl %eax,%eax
	jz Lend_noframe
	movl %eax,%ebp
	leave
	ret
Lend_noframe:
	ret
