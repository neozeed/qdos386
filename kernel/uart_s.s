/*
 * QDOS/386 R14 - IRQ3 (vector 0x23, COM2) and IRQ4 (vector 0x24, COM1) entry stubs, the irq1_stub pattern of
 * kernel/kctx.s: save the registers, move to the kernel stack when interrupted outside it, call the C handler
 * (kernel/uart.c, which sends the EOI and never schedules), return by popal/iret.  Linked only into R14 kernels.
 */
.text
.globl _irq3_stub
_irq3_stub:
	pushal
	movl %esp,%eax
	cmpl _kx_lo,%eax
	jb u3s
	cmpl _kx_hi,%eax
	jb u3k
u3s:	movl _kx_entry,%esp
u3k:	pushl %eax
	call _ser_irq3
	popl %esp
	popal
	iret

.globl _irq4_stub
_irq4_stub:
	pushal
	movl %esp,%eax
	cmpl _kx_lo,%eax
	jb u4s
	cmpl _kx_hi,%eax
	jb u4k
u4s:	movl _kx_entry,%esp
u4k:	pushl %eax
	call _ser_irq4
	popl %esp
	popal
	iret
