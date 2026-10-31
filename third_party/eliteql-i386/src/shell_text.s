# Original shell.s text/font and formatting. All entry points use cdecl.
# asm_text_at(screen,text,y,x); asm_text(screen,text,y); asm_decimal(dst,value,digits).
.text
.globl _asm_labels
_asm_labels:
 ret
.globl _asm_text
_asm_text:
 pushl $32
 pushl 16(%esp)
 pushl 16(%esp)
 pushl 16(%esp)
 call _asm_text_at
 addl $16,%esp
 ret
.globl _asm_text_at
_asm_text_at:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $24,%esp
 movl 8(%ebp),%edi
 movl 12(%ebp),%esi
 movzwl 16(%ebp),%eax
 movl %eax,-16(%ebp)
 movzwl 20(%ebp),%eax
 movl %eax,-20(%ebp)
text_character:
 cmpw $251,-20(%ebp)
 ja text_done
 cmpw $185,-16(%ebp)
 ja text_done
 movzbl (%esi),%eax
 incl %esi
 testb %al,%al
 jz text_done
 subw $32,%ax
 movzwl %ax,%eax
 imull $7,%eax,%eax
 movswl %ax,%eax
 leal asm_font(%eax),%ebx
 movl $0,-24(%ebp)
text_row:
 movzbl (%ebx),%eax
 incl %ebx
 movl %eax,-32(%ebp)
 movl $0,-28(%ebp)
text_bit:
 testl $16,-32(%ebp)
 jz text_next_bit
 movl -16(%ebp),%eax
 addw -24(%ebp),%ax
 movzwl %ax,%eax
 shll $7,%eax
 movl -20(%ebp),%edx
 addw -28(%ebp),%dx
 movzwl %dx,%edx
 movl %edx,%ecx
 shrw $2,%cx
 addw %cx,%cx
 addw %cx,%ax
 andl $3,%edx
 movw text_masks(,%edx,2),%dx
 orw %dx,(%edi,%eax)
text_next_bit:
 shlw $1,-32(%ebp)
 incl -28(%ebp)
 cmpl $5,-28(%ebp)
 jl text_bit
 incl -24(%ebp)
 cmpl $7,-24(%ebp)
 jl text_row
 addw $6,-20(%ebp)
 jmp text_character
text_done:
 addl $24,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.globl _asm_clear_view
_asm_clear_view:
 pushl %edi
 movl 8(%esp),%edi
 movl $6144,%ecx
 xorl %eax,%eax
 cld
 rep
 stosl
 popl %edi
 ret
.globl _asm_decimal
_asm_decimal:
 pushl %ebx
 pushl %edi
 movl 12(%esp),%edi
 movl 16(%esp),%eax
 movzwl 20(%esp),%ecx
 movswl %cx,%edx
 addl %edx,%edi
 movl $10,%ebx
asm_decimal_loop:
 xorl %edx,%edx
 divl %ebx
 addb $48,%dl
 decl %edi
 movb %dl,(%edi)
 decw %cx
 jnz asm_decimal_loop
 popl %edi
 popl %ebx
 ret
.data
text_masks: .word 0xc080,0x3020,0x0c08,0x0302
.globl asm_font
asm_font:
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,4,4,31,4,4,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,31,0,0,0
 .byte 0,0,0,0,0,4,4
 .byte 1,2,2,4,8,8,16
 .byte 14,17,19,21,25,17,14
 .byte 4,12,4,4,4,4,14
 .byte 14,17,1,2,4,8,31
 .byte 30,1,1,14,1,1,30
 .byte 2,6,10,18,31,2,2
 .byte 31,16,16,30,1,1,30
 .byte 14,16,16,30,17,17,14
 .byte 31,1,2,4,8,8,8
 .byte 14,17,17,14,17,17,14
 .byte 14,17,17,15,1,1,14
 .byte 0,4,4,0,4,4,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 16,8,4,2,4,8,16
 .byte 14,17,1,2,4,0,4
 .byte 0,0,0,0,0,0,0
 .byte 14,17,17,31,17,17,17
 .byte 30,17,17,30,17,17,30
 .byte 15,16,16,16,16,16,15
 .byte 30,17,17,17,17,17,30
 .byte 31,16,16,30,16,16,31
 .byte 31,16,16,30,16,16,16
 .byte 15,16,16,23,17,17,15
 .byte 17,17,17,31,17,17,17
 .byte 31,4,4,4,4,4,31
 .byte 7,2,2,2,18,18,12
 .byte 17,18,20,24,20,18,17
 .byte 16,16,16,16,16,16,31
 .byte 17,27,21,21,17,17,17
 .byte 17,25,21,19,17,17,17
 .byte 14,17,17,17,17,17,14
 .byte 30,17,17,30,16,16,16
 .byte 14,17,17,17,21,18,13
 .byte 30,17,17,30,20,18,17
 .byte 15,16,16,14,1,1,30
 .byte 31,4,4,4,4,4,4
 .byte 17,17,17,17,17,17,14
 .byte 17,17,17,17,17,10,4
 .byte 17,17,17,21,21,21,10
 .byte 17,17,10,4,10,17,17
 .byte 17,17,10,4,4,4,4
 .byte 31,1,2,4,8,16,31
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0
 .byte 0,0,0,0,0,0,0

