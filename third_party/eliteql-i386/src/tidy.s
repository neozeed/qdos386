# RC6 tidy.s: exact integer TIDY/NORM/TIS rounding and signed-zero rules.
# tidy_mult: eax/edx bytes -> eax encoded word; clobbers ebx/edx.
# normalize_vector: esi points at 3 strided high bytes; advances esi by 6.
# divide_byte_fraction: eax byte, ebx divisor -> eax, edx sign, ecx quotient.
.text
.globl _tidy_ship
_tidy_ship:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $20,%esp
 movl 8(%ebp),%esi
 movl %esi,-16(%ebp)
 addl $10,%esi
 call normalize_vector
 movl -16(%ebp),%esi
 movl $0,-20(%ebp)
 testb $96,10(%esi)
 jne tidy_x
 movl $2,-20(%ebp)
 testb $96,12(%esi)
 jne tidy_y
 movl $4,-20(%ebp)
 movl $0,-24(%ebp)
 movl $2,-28(%ebp)
 jmp tidy_project
tidy_x:
 movl $2,-24(%ebp)
 movl $4,-28(%ebp)
 jmp tidy_project
tidy_y:
 movl $0,-24(%ebp)
 movl $4,-28(%ebp)
tidy_project:
 movl -24(%ebp),%ecx
 movb 16(%esi,%ecx),%al
 movb 10(%esi,%ecx),%dl
 call tidy_mult
 pushl %eax
 movl -28(%ebp),%ecx
 movb 16(%esi,%ecx),%al
 movb 10(%esi,%ecx),%dl
 call tidy_mult
 popl %edx
 call sign_add
 xorw $32768,%ax
 movl %eax,%ecx
 shrw $8,%cx
 movl -20(%ebp),%edi
 movzbl 10(%esi,%edi),%edx
 xorb %dl,%cl
 andl $128,%ecx
 andl $32767,%eax
 andl $127,%edx
 je tidy_divzero
 movl %edx,%ebx
 xorl %edx,%edx
 divl %ebx
 jmp tidy_divsign
tidy_divzero:
 movw $255,%ax
tidy_divsign:
 orb %cl,%al
 movb %al,16(%esi,%edi)
 leal 16(%esi),%esi
 call normalize_vector
 movl $0,-32(%ebp)
tidy_cross:
 movl -16(%ebp),%esi
 movl -32(%ebp),%ecx
 movzbl cross_offsets+1(%ecx),%edx
 movb 16(%esi,%edx),%al
 movzbl cross_offsets(%ecx),%edx
 movb 10(%esi,%edx),%dl
 call tidy_mult
 pushl %eax
 movl -32(%ebp),%ecx
 movzbl cross_offsets(%ecx),%edx
 movb 16(%esi,%edx),%al
 xorb $128,%al
 movzbl cross_offsets+1(%ecx),%edx
 movb 10(%esi,%edx),%dl
 call tidy_mult
 popl %edx
 call sign_add
 shrw $8,%ax
 movl $96,%ebx
 call divide_byte_fraction
 xorb $128,%al
 movl -32(%ebp),%ecx
 movzbl cross_offsets+2(%ecx),%edx
 movb %al,(%esi,%edx)
 addl $3,-32(%ebp)
 cmpl $9,-32(%ebp)
 jb tidy_cross
 movl $14,%eax
tidy_clear:
 movb $0,9(%esi,%eax)
 subl $2,%eax
 jns tidy_clear
 addl $20,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.globl tidy_mult
tidy_mult:
 movw %ax,%bx
 xorw %dx,%bx
 andl $128,%ebx
 shll $8,%ebx
 andl $127,%eax
 andl $127,%edx
 je tidy_mult_zero
 imull %edx,%eax
 orw %bx,%ax
 ret
tidy_mult_zero:
 xorl %eax,%eax
 ret
.globl normalize_vector
normalize_vector:
 movzbl (%esi),%eax
 andl $127,%eax
 imull %eax,%eax
 movl %eax,%edi
 movzbl 2(%esi),%eax
 andl $127,%eax
 imull %eax,%eax
 addl %eax,%edi
 movzbl 4(%esi),%eax
 andl $127,%eax
 imull %eax,%eax
 addl %eax,%edi
 xorl %ebx,%ebx
 movl $128,%ecx
tidy_sqrt:
 movl %ebx,%eax
 orl %ecx,%eax
 movl %eax,%edx
 imull %eax,%eax
 cmpl %edi,%eax
 ja tidy_sqrt_next
 movl %edx,%ebx
tidy_sqrt_next:
 shrl $1,%ecx
 jne tidy_sqrt
 movl $2,%edi
tidy_norm_component:
 movzbl (%esi),%eax
 movl %eax,%edx
 andl $127,%edx
 cmpw %bx,%dx
 jae tidy_norm_saturated
 call divide_byte_fraction
 shrw $2,%cx
 movw %cx,%ax
 addw $1,%ax
 shrw $1,%ax
 addw %cx,%ax
 orw %dx,%ax
 jmp tidy_norm_store
tidy_norm_saturated:
 andl $128,%eax
 orl $96,%eax
tidy_norm_store:
 movb %al,(%esi)
 addl $2,%esi
 decl %edi
 jns tidy_norm_component
 ret
divide_byte_fraction:
 pushl %edi
 movl %eax,%edx
 andl $128,%edx
 andl $127,%eax
 xorl %ecx,%ecx
 movl $7,%edi
tidy_fraction_loop:
 addb %al,%al
 addw %cx,%cx
 cmpb %bl,%al
 jb tidy_fraction_next
 subb %bl,%al
 addw $1,%cx
tidy_fraction_next:
 decl %edi
 jns tidy_fraction_loop
 movw %cx,%ax
 orw %dx,%ax
 popl %edi
 ret
.data
cross_offsets: .byte 2,4,22,4,0,24,0,2,26
