/* Apollo R6 MBR: relocate, require one active FAT16 primary, EDD-only VBR read. */
.code16
.section .text
.global start
start:
 cli
 xorw %ax,%ax
 movw %ax,%ds
 movw %ax,%es
 movw %ax,%ss
 movw $0x7c00,%sp
 cld
 movw $0x7c00,%si
 movw $0x600,%di
 movw $256,%cx
 rep movsw
 ljmp $0,$relocated
relocated:
 sti
 movb %dl,drive
 cmpw $0xaa55,0x7fe
 jne bad_m
 movw $table,%si
 movw $4,%cx
 xorw %bp,%bp
 xorw %di,%di
scan:
 cmpb $0,(%si)
 je next
 cmpb $0x80,(%si)
 jne bad_a
 incw %bp
 movw %si,%di
next:
 addw $16,%si
 loop scan
 cmpw $1,%bp
 jne bad_a
 movw %di,%si
 movb 4(%si),%al
 cmpb $4,%al
 je type_ok
 cmpb $6,%al
 je type_ok
 cmpb $14,%al
 jne bad_p
type_ok:
 movl 8(%si),%eax
 testl %eax,%eax
 jz bad_p
 movl 12(%si),%ecx
 testl %ecx,%ecx
 jz bad_p
 addl %eax,%ecx
 jc bad_p
 cmpl $0x10000000,%ecx
 ja bad_p
 movl %eax,dap+8
 pushw %si
 movb drive,%dl
 movw $0x55aa,%bx
 movw $0x4100,%ax
 int $0x13
 jc bad_r
 cmpw $0xaa55,%bx
 jne bad_r
 testw $1,%cx
 jz bad_r
 movb drive,%dl
 movw $dap,%si
 movw $0x4200,%ax
 int $0x13
 jc bad_r
 testb %ah,%ah
 jnz bad_r
 cmpw $1,dap+2
 jne bad_r
 cmpw $0xaa55,0x7dfe
 jne bad_p
 popw %si
 movb drive,%dl
 ljmp $0,$0x7c00
bad_m: movb $'M',%al; jmp fail
bad_a: movb $'A',%al; jmp fail
bad_p: movb $'P',%al; jmp fail
bad_r: movb $'R',%al
fail:
 movb %al,errcode
 movw $message,%si
print:
 lodsb
 testb %al,%al
 jz code
 outb %al,$0xe9
 movw $0x402,%dx
 outb %al,%dx
 movb $0x0e,%ah
 xorw %bx,%bx
 int $0x10
 jmp print
code:
 movb errcode,%al
 movw $0x402,%dx
 outb %al,%dx
 cli
halt: hlt; jmp halt
message: .asciz "R6MBR:"
drive: .byte 0
errcode: .byte 0
.balign 4
dap: .byte 16,0; .word 1,0x7c00,0; .long 0,0
.org 440
 .long 0 /* deterministic disk ID installed by builder */
 .word 0
table: .space 64
 .word 0xaa55
