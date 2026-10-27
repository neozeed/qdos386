/ jmp.s - QDOS/386 libc (R11D): native i386 setjmp and longjmp.
/ Historical GAS 1.38 syntax.  The buffer (jmp_buf, six longs) holds:
/   0 ebx   4 esi   8 edi   12 ebp   16 esp (caller's, after the return)   20 eip
/ These are the registers that the GCC 1.40 i386 calling convention keeps
/ across a call.  The x87 state is not saved (see docs/R11D-LIBC-PORT-MAP.md).

	.globl _setjmp
	.align 2
_setjmp:
	movl 4(%esp),%eax		/* env */
	movl %ebx,0(%eax)
	movl %esi,4(%eax)
	movl %edi,8(%eax)
	movl %ebp,12(%eax)
	leal 4(%esp),%ecx		/* esp as the caller will see it */
	movl %ecx,16(%eax)
	movl (%esp),%ecx		/* return address */
	movl %ecx,20(%eax)
	xorl %eax,%eax
	ret

	.globl _longjmp
	.align 2
_longjmp:
	movl 4(%esp),%edx		/* env */
	movl 8(%esp),%eax		/* val */
	testl %eax,%eax
	jne Ljval
	movl $1,%eax			/* longjmp(env, 0) makes setjmp return 1 */
Ljval:
	movl 0(%edx),%ebx
	movl 4(%edx),%esi
	movl 8(%edx),%edi
	movl 12(%edx),%ebp
	movl 16(%edx),%esp
	jmp *20(%edx)
