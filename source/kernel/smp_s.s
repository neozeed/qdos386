/*
 * QDOS/386 R16 - application-processor trampoline, LAPIC MMIO helpers and the AP (CPU 1) timer stub.
 * The AP, unlike the R15 AMP worker, is a full kernel CPU: it loads the kernel GDT/IDT, its own TSS (0x38), enables
 * paging with the template directory, and runs the ordinary scheduler under the BKL.  docs/R16-ARCHITECTURE.md.
 *
 * GAS 1.38 has no .code16: the real-mode trampoline is hand-encoded (as platform/pc/boot2.s).  CPU 0 copies
 * _smp_trampoline to SMP_TRAMP_PA (0x9000) and fills _smp_ap_pdbr / _smp_ap_gdtr / _smp_ap_idtr / _smp_ap_esp
 * before the SIPI.
 */
	.data
	.globl _smp_trampoline
	.globl _smp_trampoline_end
_smp_trampoline:
	.byte 0xFA			/* 00 cli */
	.byte 0xFC			/* 01 cld */
	.byte 0x8C,0xC8			/* 02 mov ax,cs */
	.byte 0x8E,0xD8			/* 04 mov ds,ax */
	.byte 0x66,0x0F,0x01,0x16	/* 06 o32 lgdt [0x0028] (trampoline temp GDT) */
	.word 0x0028
	.byte 0x0F,0x20,0xC0		/* 0C mov eax,cr0 */
	.byte 0x66,0x83,0xC8,0x01	/* 0F or eax,1 (PE) */
	.byte 0x0F,0x22,0xC0		/* 13 mov cr0,eax */
	.byte 0x66,0xEA			/* 16 o32 jmp far 0008:_smp_ap_pm */
	.long _smp_ap_pm
	.word 0x0008
	.byte 0xF4			/* 1E hlt */
	.byte 0x90			/* 1F nop */
	.word 23			/* 20 temp gdt limit (unused slot padding to 0x28) */
	.long 0
	.word 0
	.word 23			/* 28 temp gdt descriptor: limit */
	.long 0x00009030		/* 2A base = SMP_TRAMP_PA + 0x30 */
	.word 0				/* 2E */
	.long 0,0			/* 30 null */
	.word 0xFFFF,0x0000		/* 38 code 0x08 flat 32-bit */
	.byte 0x00,0x9A,0xCF,0x00
	.word 0xFFFF,0x0000		/* 40 data 0x10 flat */
	.byte 0x00,0x92,0xCF,0x00
_smp_trampoline_end:

	.align 4
	.globl _smp_ap_pdbr
_smp_ap_pdbr:	.long 0			/* template CR3 (filled by CPU 0) */
	.globl _smp_ap_gdtr
_smp_ap_gdtr:	.long 0			/* &gdtr (6-byte) of the kernel GDT */
	.globl _smp_ap_idtr
_smp_ap_idtr:	.long 0			/* &idtr of the kernel IDT */
	.globl _smp_ap_esp
_smp_ap_esp:	.long 0			/* top of the AP kernel stack */
	.globl _smp_ap_entered
_smp_ap_entered:	.long 0		/* set non-zero by the AP once in 32-bit kernel mode */

	.text
/*
 * 32-bit entry (still on the trampoline's flat GDT, no stack).  Switch to the real kernel GDT/IDT, the AP stack,
 * enable paging with the template directory, load TR = 0x38, then call the C entry smp_ap_main (never returns).
 */
	.globl _smp_ap_pm
_smp_ap_pm:
	movw $0x10,%ax
	movw %ax,%ds
	movw %ax,%es
	movw %ax,%ss
	movw %ax,%fs
	movw %ax,%gs
	movl _smp_ap_esp,%esp
	movl _smp_ap_gdtr,%eax
	lgdt (%eax)
	ljmp $0x08,$smpap1
smpap1:
	movw $0x10,%ax
	movw %ax,%ds
	movw %ax,%es
	movw %ax,%ss
	movw %ax,%fs
	movw %ax,%gs
	movl _smp_ap_idtr,%eax
	lidt (%eax)
	movl _smp_ap_pdbr,%eax		/* enable paging with the shared template directory */
	movl %eax,%cr3
	movl %cr0,%eax
	orl $0x80000000,%eax
	movl %eax,%cr0
	jmp smpap2
smpap2:
	movw $0x38,%ax			/* TR = the AP TSS (selector 0x38) */
	ltr %ax
	movl $1,_smp_ap_entered
	call _smp_ap_main
	cli
smpaphlt:
	hlt
	jmp smpaphlt

/*
 * AP LAPIC timer (vector 0x40): the IRQ0_stub shape, but it calls smp_ap_timer_tick (LAPIC EOI, no PIT/clock/poll)
 * and shares resume_dispatch in kctx3.s.  str sets EBP = 16 (CPU 1) so the per-CPU block and BKL are correct.
 */
	.globl _smp_ap_timer_stub
_smp_ap_timer_stub:
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
	jb at_s
	cmpl _kx_pc+4(%ebp),%eax
	jb at_k
at_s:	movl _kx_pc+8(%ebp),%esp
at_k:	pushl %eax
	call _bkl_enter
	call _smp_ap_timer_resume	/* returns the resume stack pointer in EAX, sets kx_pc[cpu][3] */
	addl $4,%esp
	jmp resume_dispatch

/* ---- LAPIC MMIO and small helpers (physical address passed in) ---- */
	.globl _smp_mmio_rd
_smp_mmio_rd:
	movl 4(%esp),%eax
	movl (%eax),%eax
	ret
	.globl _smp_mmio_wr
_smp_mmio_wr:
	movl 4(%esp),%edx
	movl 8(%esp),%eax
	movl %eax,(%edx)
	ret
	.globl _smp_has_cpuid
_smp_has_cpuid:
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
	.globl _smp_cpuid
_smp_cpuid:
	pushl %ebx
	pushl %edi
	movl 12(%esp),%eax
	movl 16(%esp),%edi
	xorl %ecx,%ecx
	.byte 0x0F,0xA2
	movl %eax,(%edi)
	movl %ebx,4(%edi)
	movl %ecx,8(%edi)
	movl %edx,12(%edi)
	popl %edi
	popl %ebx
	ret
	.globl _smp_rdmsr
_smp_rdmsr:
	movl 4(%esp),%ecx
	.byte 0x0F,0x32
	movl 8(%esp),%ecx
	movl %edx,(%ecx)
	ret
