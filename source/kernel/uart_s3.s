/*
 * QDOS/386 R14 - IRQ3 (vector 0x23, COM2) and IRQ4 (vector 0x24, COM1) entry stubs, the irq1_stub pattern of
 * kernel/kctx.s: save the registers, move to the kernel stack when interrupted outside it, call the C handler
 * (kernel/uart.c, which sends the EOI and never schedules), return by popal/iret.  R16 copy (kernel/uart_s3.s): kernel DS/ES on entry, per-CPU kx block via str, user segments restored for a CPL3 frame on return; linked only into the R16 kernel.
 */
.text
.globl _irq3_stub
_irq3_stub:
	pushal
	movw $0x10,%dx
	movw %dx,%ds
	movw %dx,%es
	str %cx
	xorl %ebp,%ebp
	cmpw $0x38,%cx
	jne 9f
	movl $16,%ebp
9:
	movl %esp,%eax
	cmpl _kx_pc(%ebp),%eax
	jb u3s
	cmpl _kx_pc+4(%ebp),%eax
	jb u3k
u3s:	movl _kx_pc+8(%ebp),%esp
u3k:	pushl %eax
	call _ser_irq3
	popl %esp
	testl $3,36(%esp)
	jz ur0
	movw $0x33,%dx
	movw %dx,%ds
	movw %dx,%es
	movw %dx,%fs
	movw %dx,%gs
ur0:
	popal
	iret

.globl _irq4_stub
_irq4_stub:
	pushal
	movw $0x10,%dx
	movw %dx,%ds
	movw %dx,%es
	str %cx
	xorl %ebp,%ebp
	cmpw $0x38,%cx
	jne 9f
	movl $16,%ebp
9:
	movl %esp,%eax
	cmpl _kx_pc(%ebp),%eax
	jb u4s
	cmpl _kx_pc+4(%ebp),%eax
	jb u4k
u4s:	movl _kx_pc+8(%ebp),%esp
u4k:	pushl %eax
	call _ser_irq4
	popl %esp
	testl $3,36(%esp)
	jz ur1
	movw $0x33,%dx
	movw %dx,%ds
	movw %dx,%es
	movw %dx,%fs
	movw %dx,%gs
ur1:
	popal
	iret
