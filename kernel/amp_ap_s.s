/*
 * QDOS/386 R15 AMP0 - WORKER (application processor) assembly.  Runs only on the worker CPU, never on CPU 0, except
 * that CPU 0 COPIES amp_trampoline to AMP_TRAMP_PA and reads ap_exc_table to fill the AP IDT before the start-up IPI.
 * Static rule (tests/r15/check-ap-objects.py): this object and kernel/amp_ap.c reference no symbol outside themselves,
 * and contain no INT instruction - the worker can never enter the QDOS kernel.
 *
 * Start-up path (docs/R15-AMP-ARCHITECTURE.md):
 *   SIPI vector 0x08 -> real mode at 0800:0000 (amp_trampoline, copied to 0x8000): load the trampoline GDT, set CR0.PE,
 *   far jump to ap_entry32 (selector 0x08, flat).  ap_entry32: flat data segments, the worker stack, CR0 = PE|MP|EM|NE
 *   (caches on: CD/NW cleared; EM so any x87 instruction faults instead of touching state nobody saved), PAGING STAYS
 *   OFF (physical addressing: the worker never uses CPU 0's page tables), the worker's own GDT and IDT, then ap_main.
 *   Exceptions: 32 interrupt gates to ap_exc_common -> ap_fault (records, then halts THIS CPU only).
 * GAS 1.38 has no .code16: the real-mode bytes are encoded by hand (as platform/pc/boot2.s does).
 */
	.data
/* ---- real-mode trampoline, copied to AMP_TRAMP_PA (0x8000), entered at 0800:0000 ---- */
	.globl _amp_trampoline
	.globl _amp_trampoline_end
_amp_trampoline:
	.byte 0xFA			/* 00 cli */
	.byte 0xFC			/* 01 cld */
	.byte 0x8C,0xC8			/* 02 mov ax,cs */
	.byte 0x8E,0xD8			/* 04 mov ds,ax */
	.byte 0x66,0x0F,0x01,0x16	/* 06 o32 lgdt [0x0020] */
	.word 0x0020
	.byte 0x0F,0x20,0xC0		/* 0C mov eax,cr0 */
	.byte 0x66,0x83,0xC8,0x01	/* 0F or eax,1 */
	.byte 0x0F,0x22,0xC0		/* 13 mov cr0,eax */
	.byte 0x66,0xEA			/* 16 o32 jmp far 0008:ap_entry32 */
	.long _ap_entry32
	.word 0x0008
	.byte 0xF4,0xF4			/* 1E hlt (never reached) */
	.word 23			/* 20 gdt limit */
	.long 0x00008028		/* 22 gdt base = AMP_TRAMP_PA + 0x28 */
	.word 0				/* 26 */
	.long 0,0			/* 28 null */
	.word 0xFFFF,0x0000		/* 30 code 0x08: base 0, limit 4 GiB, 32-bit */
	.byte 0x00,0x9A,0xCF,0x00
	.word 0xFFFF,0x0000		/* 38 data 0x10 */
	.byte 0x00,0x92,0xCF,0x00
_amp_trampoline_end:

/* ---- the worker's own descriptor tables and stack pointer ---- */
	.align 4
	.globl _amp_ap_gdt
_amp_ap_gdt:
	.long 0,0
	.word 0xFFFF,0x0000
	.byte 0x00,0x9A,0xCF,0x00
	.word 0xFFFF,0x0000
	.byte 0x00,0x92,0xCF,0x00
	.align 4
	.word 0
_amp_ap_gdtdesc:
	.word 23
	.long _amp_ap_gdt
	.align 4
	.globl _amp_ap_idt
_amp_ap_idt:
	.space 256
	.align 4
	.word 0
_amp_ap_idtdesc:
	.word 255
	.long _amp_ap_idt
	.align 4
	.globl _amp_ap_stack_top_v
_amp_ap_stack_top_v:
	.long 0
	.globl _ap_exc_table
_ap_exc_table:
	.long apx0, apx1, apx2, apx3, apx4, apx5, apx6, apx7
	.long apx8, apx9, apx10, apx11, apx12, apx13, apx14, apx15
	.long apx16, apx17, apx18, apx19, apx20, apx21, apx22, apx23
	.long apx24, apx25, apx26, apx27, apx28, apx29, apx30, apx31

	.text
/* ---- 32-bit entry from the trampoline (CS = trampoline selector 0x08, no stack yet) ---- */
	.globl _ap_entry32
_ap_entry32:
	movw $0x10,%ax
	movw %ax,%ds
	movw %ax,%es
	movw %ax,%ss
	movw %ax,%fs
	movw %ax,%gs
	movl _amp_ap_stack_top_v,%esp
	movl %cr0,%eax
	andl $0x9FFFFFFF,%eax
	orl $0x00000026,%eax
	movl %eax,%cr0
	movl $_amp_ap_gdtdesc,%eax
	lgdt (%eax)
	ljmp $0x08,$ape1
ape1:
	movw $0x10,%ax
	movw %ax,%ds
	movw %ax,%es
	movw %ax,%ss
	movw %ax,%fs
	movw %ax,%gs
	movl $_amp_ap_idtdesc,%eax
	lidt (%eax)
	call _ap_main
	.globl _ap_halt_forever
_ap_halt_forever:
	cli
	hlt
	jmp _ap_halt_forever

/* ---- exceptions: frame = pushal, vector, error (0 if none), eip, cs, eflags ---- */
apx0:
	pushl $0
	pushl $0
	jmp ap_exc_common
apx1:
	pushl $0
	pushl $1
	jmp ap_exc_common
apx2:
	pushl $0
	pushl $2
	jmp ap_exc_common
apx3:
	pushl $0
	pushl $3
	jmp ap_exc_common
apx4:
	pushl $0
	pushl $4
	jmp ap_exc_common
apx5:
	pushl $0
	pushl $5
	jmp ap_exc_common
apx6:
	pushl $0
	pushl $6
	jmp ap_exc_common
apx7:
	pushl $0
	pushl $7
	jmp ap_exc_common
apx8:
	pushl $8
	jmp ap_exc_common
apx9:
	pushl $0
	pushl $9
	jmp ap_exc_common
apx10:
	pushl $10
	jmp ap_exc_common
apx11:
	pushl $11
	jmp ap_exc_common
apx12:
	pushl $12
	jmp ap_exc_common
apx13:
	pushl $13
	jmp ap_exc_common
apx14:
	pushl $14
	jmp ap_exc_common
apx15:
	pushl $0
	pushl $15
	jmp ap_exc_common
apx16:
	pushl $0
	pushl $16
	jmp ap_exc_common
apx17:
	pushl $17
	jmp ap_exc_common
apx18:
	pushl $0
	pushl $18
	jmp ap_exc_common
apx19:
	pushl $0
	pushl $19
	jmp ap_exc_common
apx20:
	pushl $0
	pushl $20
	jmp ap_exc_common
apx21:
	pushl $0
	pushl $21
	jmp ap_exc_common
apx22:
	pushl $0
	pushl $22
	jmp ap_exc_common
apx23:
	pushl $0
	pushl $23
	jmp ap_exc_common
apx24:
	pushl $0
	pushl $24
	jmp ap_exc_common
apx25:
	pushl $0
	pushl $25
	jmp ap_exc_common
apx26:
	pushl $0
	pushl $26
	jmp ap_exc_common
apx27:
	pushl $0
	pushl $27
	jmp ap_exc_common
apx28:
	pushl $0
	pushl $28
	jmp ap_exc_common
apx29:
	pushl $0
	pushl $29
	jmp ap_exc_common
apx30:
	pushl $0
	pushl $30
	jmp ap_exc_common
apx31:
	pushl $0
	pushl $31
	jmp ap_exc_common
ap_exc_common:
	pushal
	pushl %esp
	call _ap_fault
	jmp _ap_halt_forever

/* ---- worker primitives (out of line: each call is a compiler barrier) ---- */
	.globl _ap_load
_ap_load:
	movl 4(%esp),%eax
	movl (%eax),%eax
	ret
	.globl _ap_xchg
_ap_xchg:				/* unsigned ap_xchg(volatile unsigned *p, unsigned v): locked, returns the old value */
	movl 4(%esp),%edx
	movl 8(%esp),%eax
	xchgl %eax,(%edx)
	ret
	.globl _ap_cas
_ap_cas:				/* unsigned ap_cas(p, old, new): lock cmpxchg; returns the value found (== old: swapped) */
	movl 4(%esp),%edx
	movl 8(%esp),%eax
	movl 12(%esp),%ecx
	.byte 0xF0,0x0F,0xB1,0x0A	/* lock cmpxchgl %ecx,(%edx) */
	ret
	.globl _ap_pause
_ap_pause:
	.byte 0xF3,0x90			/* pause (rep nop) */
	ret
	.globl _ap_cpuid1_ebx
_ap_cpuid1_ebx:
	pushl %ebx
	movl $1,%eax
	.byte 0x0F,0xA2			/* cpuid */
	movl %ebx,%eax
	popl %ebx
	ret
	.globl _ap_rdmsr_lo
_ap_rdmsr_lo:
	movl 4(%esp),%ecx
	.byte 0x0F,0x32			/* rdmsr */
	ret
	.globl _ap_read_cr2
_ap_read_cr2:
	movl %cr2,%eax
	ret
	.globl _ap_invalid_op
_ap_invalid_op:
	.byte 0x0F,0x0B			/* ud2: #UD on the worker (AMP_CMD_T_FAULT) */
	ret
