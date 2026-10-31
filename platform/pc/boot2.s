/*
 * QDOS/386 R11K (PC) - 512-byte boot sector, multi-transfer loader.
 *
 * Replaces platform/pc/boot.s ONLY when a PC image is built with PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py (boot.s and
 * make-image.py are untouched and remain the default).  Same disk layout as before: sector 0 = this
 * boot sector, sectors 1..K = kernel, K+1..K+X = transfer area; no partition table, no second stage.
 *
 * Why the old loader stopped at 120 sectors: it read kernel + transfer with ONE INT 13h AH=42h call into
 * 1000:0000.  A transfer cannot cross the 64 KiB segment end, so 128 sectors is the hard limit (observed:
 * 129 hangs on Tiny386 and on QEMU/SeaBIOS) and make-image.py kept 8 sectors of margin (0xF000 bytes).
 * See docs/R11K-BOOT-AUDIT.md.
 *
 * Steps (all INT 13h results are checked; every failure ends in the diagnostic at 'pmfail'):
 *   1. validate the sector counts patched into this sector by make-image2.py:
 *        K >= 1, 1 <= X <= 512, K + X <= 896 (the low-memory buffer below)
 *   2. check the BIOS-reported conventional memory (0040:0013) covers the buffer
 *   3. read K + X sectors with INT 13h AH=42h in chunks of at most 64 sectors (32 KiB, never
 *      crossing a 64 KiB boundary) into consecutive low memory starting at 1000:0000; the buffer
 *      is 0x10000 .. 0x7FFFF (448 KiB).  Chunk count, LBA and segment advance after each call.
 *   4. CLI (no IDT exists in protected mode yet), enable A20 (port 0x92), load the flat GDT, set CR0.PE
 *   5. protected mode: verify A20 really is on (0x500 vs 0x100500 must not alias), verify the 32-bit
 *      rotate-and-add checksum of all K + X sectors against the value make-image2.py stored, copy
 *      the kernel to 0x100000 and the transfer area to 0x280000 (R9 images: sed makes it 0x340000),
 *      jump to 0x100000.
 *
 * Diagnostic: "QBOOT:<c>\n" on the Tiny386 debug port 0x402, COM1 (0x3F8) and the VGA text
 * buffer (top left), then CLI/HLT forever.  <c> is
 *     N invalid sector counts      M too little conventional memory   R BIOS read error
 *     A A20 did not come on        S checksum mismatch (truncated or corrupt image)
 *
 * GAS 1.38 has no .code16, so the real-mode part is encoded with .byte/.word, as boot.s does; the
 * encoding is checked by tests/host/r11k/boot2-disasm.sh.
 *
 * Fixed layout (offsets from 0x7C00):
 *   0x1B0 drive   0x1B2 DAP (16 bytes)   0x1C2 ksec (word)   0x1C4 xsec (word)   0x1C6 checksum (dword)
 *   0x1CA "QBOOT2" (loader tag)
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
	.byte 0xFB			/* sti (BIOS disk services want interrupts) */
	.byte 0x88,0x16			/* mov [drive],dl */
	.word 0x7C00+0x1B0
	/* 1. validate counts */
	.byte 0xA1			/* mov ax,[ksec] */
	.word 0x7C00+0x1C2
	.byte 0x85,0xC0			/* test ax,ax */
	.byte 0x74			/* jz bad_n */
	.byte bad_n - . - 1
	.byte 0x8B,0x0E			/* mov cx,[xsec] */
	.word 0x7C00+0x1C4
	.byte 0xE3			/* jcxz bad_n */
	.byte bad_n - . - 1
	.byte 0x81,0xF9			/* cmp cx,512 */
	.word 512
	.byte 0x77			/* ja bad_n */
	.byte bad_n - . - 1
	.byte 0x01,0xC8			/* add ax,cx  (K + X) */
	.byte 0x72			/* jc bad_n */
	.byte bad_n - . - 1
	.byte 0x3D			/* cmp ax,896 */
	.word 896
	.byte 0x77			/* ja bad_n */
	.byte bad_n - . - 1
	/* 2. conventional memory must cover 0x80000 */
	.byte 0x81,0x3E			/* cmp word [0x413],512 (KiB) */
	.word 0x0413
	.word 512
	.byte 0x72			/* jb bad_m */
	.byte bad_m - . - 1
	/* 3. chunked read: BX = sectors left, SI = DAP */
	.byte 0x89,0xC3			/* mov bx,ax */
	.byte 0xBE			/* mov si,dap */
	.word 0x7C00+0x1B2
chunk:
	.byte 0xB8			/* mov ax,64 */
	.word 64
	.byte 0x39,0xC3			/* cmp bx,ax */
	.byte 0x73,0x02			/* jae +2 */
	.byte 0x89,0xD8			/* mov ax,bx */
	.byte 0x89,0x44,0x02		/* mov [si+2],ax  (DAP count) */
	.byte 0x53			/* push bx */
	.byte 0x50			/* push ax */
	.byte 0x8A,0x16			/* mov dl,[drive] */
	.word 0x7C00+0x1B0
	.byte 0xB4,0x42			/* mov ah,0x42 */
	.byte 0xCD,0x13			/* int 0x13 */
	.byte 0x72			/* jc bad_r */
	.byte bad_r - . - 1
	.byte 0x84,0xE4			/* test ah,ah */
	.byte 0x75			/* jnz bad_r */
	.byte bad_r - . - 1
	.byte 0x58			/* pop ax  (chunk) */
	.byte 0x5B			/* pop bx  (left) */
	.byte 0xBE			/* mov si,dap  (a BIOS may not preserve SI) */
	.word 0x7C00+0x1B2
	.byte 0x29,0xC3			/* sub bx,ax */
	.byte 0x74			/* jz done */
	.byte done - . - 1
	.byte 0x01,0x44,0x08		/* add [si+8],ax    LBA low  += chunk */
	.byte 0x83,0x54,0x0A,0x00	/* adc word [si+10],0 */
	.byte 0xC1,0xE0,0x05		/* shl ax,5         chunk * 512 / 16 paragraphs */
	.byte 0x01,0x44,0x06		/* add [si+6],ax    segment += paragraphs */
	.byte 0xEB			/* jmp chunk */
	.byte chunk - . - 1
done:
	.byte 0xFA			/* cli: no IDT in protected mode */
	.byte 0x30,0xD2			/* xor dl,dl  (no error) */
	.byte 0xEB			/* jmp pmentry */
	.byte pmentry - . - 1
bad_n:	.byte 0xB2,0x4E			/* mov dl,'N' */
	.byte 0xEB
	.byte pmentry - . - 1
bad_m:	.byte 0xB2,0x4D			/* mov dl,'M' */
	.byte 0xEB
	.byte pmentry - . - 1
bad_r:	.byte 0xFA			/* cli */
	.byte 0xB2,0x52			/* mov dl,'R' */
pmentry:
	.byte 0xFA			/* cli: every path (also N and M) enters protected mode with interrupts off */
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

/* ---- protected mode (32-bit code); DL = error code or 0 ---- */
pm:
	movw $0x10,%ax
	movw %ax,%ds
	movw %ax,%es
	movw %ax,%ss
	movw %ax,%fs
	movw %ax,%gs
	movl $0x90000,%esp
	testb %dl,%dl
	jne pmfail
	/* A20 check: 0x500 and 0x100500 are the same byte when A20 is off. */
	movl $0x500,%esi
	movl $0x100500,%edi
	movl (%esi),%eax
	notl %eax
	movl %eax,(%edi)
	cmpl %eax,(%esi)
	jne a20ok
	movb $0x41,%dl			/* 'A' */
	jmp pmfail
a20ok:
	/* checksum over K + X sectors in the low buffer */
	movl (0x7C00+0x1C2),%ecx
	andl $0xffff,%ecx
	movl (0x7C00+0x1C4),%edx
	andl $0xffff,%edx
	addl %edx,%ecx
	shll $7,%ecx			/* dwords */
	movl $0x10000,%esi
	xorl %ebx,%ebx
	cld
sumloop:
	lodsl
	addl %eax,%ebx
	roll $1,%ebx
	decl %ecx
	jnz sumloop
	cmpl (0x7C00+0x1C6),%ebx
	je sumok
	movb $0x53,%dl			/* 'S' */
	jmp pmfail
sumok:
	movl (0x7C00+0x1C2),%ecx
	andl $0xffff,%ecx
	shll $7,%ecx			/* kernel dwords */
	movl $0x10000,%esi
	movl $0x100000,%edi
	rep
	movsl
	/* ESI now points at the transfer area (0x10000 + kernel bytes). */
	movl $0x280000,%edi
	movl (0x7C00+0x1C4),%ecx
	andl $0xffff,%ecx
	shll $7,%ecx			/* transfer dwords */
	rep
	movsl
	movl $0x100000,%eax
	jmp *%eax

pmfail:
	movb %dl,(0x7C00+msg+6)
	movl $(0x7C00+msg),%esi
	movl $0xB8000,%edi
	movl $8,%ecx
fail1:
	lodsb
	movw $0x402,%dx
	outb %al,%dx
	movw $0x3f8,%dx
	outb %al,%dx
	movb $0x07,%ah
	stosw
	decl %ecx
	jnz fail1
	cli
failhlt:
	hlt
	jmp failhlt
msg:	.ascii "QBOOT:?\n"

	.org 0x1B0
drive:	.byte 0
	.byte 0
dap:	.byte 0x10,0			/* size, reserved */
	.word 0				/* count (set per chunk) */
	.word 0x0000			/* offset */
	.word 0x1000			/* segment -> 0x10000 */
	.long 1				/* LBA low: first kernel sector */
	.long 0				/* LBA high */
	.org 0x1C2
ksec:	.word 0				/* patched by make-image2.py */
	.org 0x1C4
xsec:	.word 0				/* transfer sectors, patched by make-image2.py */
	.org 0x1C6
csum:	.long 0				/* checksum, patched by make-image2.py */
	.org 0x1CA
	.ascii "QBOOT2"			/* identifies this loader: make-image2.py refuses any other boot sector */
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
