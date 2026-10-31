/*
 * QDOS/386 R1 (PC) - 512-byte boot sector.
 *
 * Loaded by SeaBIOS at 0000:7C00 with DL = boot drive.  GAS 1.38 has no
 * .code16, so the real-mode instructions are encoded with .byte/.word.
 * The protected-mode part (after 'pm:') is ordinary 32-bit code.
 *
 * Steps: read the kernel with INT 13h AH=42h to 1000:0000 (phys 0x10000),
 * enable A20, load a flat GDT, set CR0.PE, copy the kernel to 0x100000 and
 * jump to it.  The kernel sector count is patched at offset 0x1C2 by
 * make-image.py.
 *
 * Fixed layout (offsets from 0x7C00):
 *   0x1B0 drive   0x1B2 DAP (16 bytes)   0x1C2 ksec (words)
 *   0x1D0 gdtdesc 0x1E0 GDT (24 bytes)   0x1FE boot signature
 */
.text
.globl start
start:
	.byte 0xFA			/* cli */
	.byte 0x31,0xC0			/* xor ax,ax */
	.byte 0x8E,0xD8			/* mov ds,ax */
	.byte 0x8E,0xC0			/* mov es,ax */
	.byte 0x8E,0xD0			/* mov ss,ax */
	.byte 0xBC
	.word 0x7C00			/* mov sp,0x7c00 */
	.byte 0xFB			/* sti */
	.byte 0x88,0x16			/* mov [drive],dl */
	.word 0x7C00+0x1B0
	.byte 0xA1			/* mov ax,[ksec] */
	.word 0x7C00+0x1C2
	.byte 0x03,0x06			/* add ax,[xsec]  (kernel + transfer sectors) */
	.word 0x7C00+0x1C4
	.byte 0xA3			/* mov [dap+2],ax */
	.word 0x7C00+0x1B2+2
	.byte 0xBE			/* mov si,dap */
	.word 0x7C00+0x1B2
	.byte 0x8A,0x16			/* mov dl,[drive] */
	.word 0x7C00+0x1B0
	.byte 0xB4,0x42			/* mov ah,0x42 */
	.byte 0xCD,0x13			/* int 0x13 */
	.byte 0x72			/* jc hang */
	.byte hang - . - 1
	.byte 0xE4,0x92			/* in al,0x92 */
	.byte 0x0C,0x02			/* or al,2 (A20) */
	.byte 0xE6,0x92			/* out 0x92,al */
	.byte 0x0F,0x01,0x16		/* lgdt [gdtdesc] */
	.word 0x7C00+0x1D0
	.byte 0x0F,0x20,0xC0		/* mov eax,cr0 */
	.byte 0x0C,0x01			/* or al,1 (PE) */
	.byte 0x0F,0x22,0xC0		/* mov cr0,eax */
	.byte 0xEA			/* jmp far 0x08:pm */
	.word 0x7C00+pm
	.word 0x0008
hang:
	.byte 0xEB,0xFE			/* jmp $ */

/* ---- protected mode (32-bit code) ---- */
pm:
	movw $0x10,%ax
	movw %ax,%ds
	movw %ax,%es
	movw %ax,%ss
	movw %ax,%fs
	movw %ax,%gs
	movl $0x90000,%esp
	movl (0x7C00+0x1C2),%ecx
	andl $0xffff,%ecx
	shll $9,%ecx			/* bytes = sectors * 512 */
	movl $0x10000,%esi
	movl $0x100000,%edi
	cld
	rep
	movsb
	/* ESI now points at the transfer area (0x10000 + kernel bytes). */
	movl $0x280000,%edi
	movl (0x7C00+0x1C4),%ecx
	andl $0xffff,%ecx
	shll $9,%ecx			/* transfer bytes = xsec * 512 */
	rep
	movsb
	movl $0x100000,%eax
	jmp *%eax

	.org 0x1B0
drive:	.byte 0
	.byte 0
dap:	.byte 0x10,0			/* size, reserved */
	.word 0				/* count (patched at run time from ksec) */
	.word 0x0000			/* offset */
	.word 0x1000			/* segment -> 0x10000 */
	.long 1				/* LBA low: first kernel sector */
	.long 0				/* LBA high */
	.org 0x1C2
ksec:	.word 0				/* patched by make-image.py */
	.org 0x1C4
 xsec:	.word 0				/* transfer sectors, patched by make-image.py */
	.org 0x1D0
gdtdesc:
	.word 24-1
	.long 0x7C00+gdt
	.org 0x1E0
gdt:
	.long 0,0			/* null */
	.word 0xFFFF,0x0000		/* code 0x08: base 0, limit 4G */
	.byte 0x00,0x9A,0xCF,0x00
	.word 0xFFFF,0x0000		/* data 0x10 */
	.byte 0x00,0x92,0xCF,0x00
	.org 0x1FE
	.byte 0x55,0xAA
