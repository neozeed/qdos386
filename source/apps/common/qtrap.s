/*
 * QDOS/386 R5 - C-callable trap wrapper for applications.
 *
 *   int qtrap(unsigned fn, unsigned a, unsigned b, unsigned c)
 *
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
