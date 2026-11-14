/* BPB bytes3..61 retained from independently formatted FAT16 volume. */
.code16
.section .text
.global start
start:
 jmp main
 nop
.org 62
main:
 cli
 xorw %ax,%ax
 movw %ax,%ds
 movw %ax,%es
 movw %ax,%ss
 movw $0x7c00,%sp
 sti
 movb %dl,drive
 movw %si,entry_ptr
 movw $dap,%si
 movw $0x4200,%ax
 int $0x13
 jc fail
 testb %ah,%ah
 jnz fail
 cmpw $32,dap+2
 jne fail
 cmpl $0x314c3652,0x1008
 jne fail_sum
 cmpl $16384,0x100c
 jne fail_sum
 movw $0x1000,%si
 movw $4096,%cx
 xorl %eax,%eax
sum:
 addl (%si),%eax
 addw $4,%si
 loop sum
 testl %eax,%eax
 jnz fail_sum
 movb drive,%dl
 movw entry_ptr,%si
 ljmp $0,$0x1000
fail_sum:
 movb $'S',errcode
fail:
 movw $message,%si
print:
 lodsb
 testb %al,%al
 jz err
 movw $0x402,%dx
 outb %al,%dx
 jmp print
err:
 movb errcode,%al
 movw $0x402,%dx
 outb %al,%dx
halt: cli; hlt; jmp halt
message: .asciz "R6VBR:"
errcode: .byte 'R'
drive: .byte 0
entry_ptr: .word 0
.balign 4
dap: .byte 16,0; .word 32,0x1000,0; .long 1,0
.org 510
 .word 0xaa55
