/* Timer bootstrap test: flat GDT, IDT gate for IRQ0 (vector 0x20),
   synthetic timer on 0x800003, wait for 5 deliveries, verify count. */
.text
.globl _main
_main:
	cli
	lgdt gdtr
	movw $0x10,%ax
	movw %ax,%ds
	movw %ax,%es
	movw %ax,%ss
	ljmp $0x08,$reload
reload:
	lidt idtr
	movl $0,ticks
	movb $1,0x800003	/* IRQ0 every 1024 steps */
	sti
wait:
	cmpl $5,ticks
	jb wait
	cli
	movzbl 0x800003,%eax	/* delivered count, low byte */
	movl %eax,%ebx
	movb $0,0x800003	/* disable timer */
	movl ticks,%ecx
	cmpl %ecx,%ebx		/* device count must match guest count */
	jne fail
	cmpl $5,%ecx
	jb fail
	movb $84,0x800001
	movb $73,0x800001
	movb $67,0x800001
	movb $75,0x800001
	movb $10,0x800001
	movl $0,%eax
	ret
fail:
	movb $70,0x800001
	movb $10,0x800001
	movl $1,%eax
	ret

timer_isr:
	incl ticks
	iret

.data
.align 4
ticks:	.long 0
gdt:
	.long 0,0
	.byte 0xff,0xff,0,0,0,0x9a,0xcf,0	/* 0x08 code */
	.byte 0xff,0xff,0,0,0,0x92,0xcf,0	/* 0x10 data */
gdt_end:
gdtr:
	.word gdt_end-gdt-1
	.long gdt
.align 4
idt:
	.space 0x100
	.word timer_isr,0x08
	.byte 0,0x8e
	.word 0
	.space 0x800-0x108-8+0
idt_end:
idtr:
	.word 0x800-1
	.long idt
