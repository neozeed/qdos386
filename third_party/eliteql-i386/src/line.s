# RC6 MODE 8 rasterizer. Same packed pixel masks and inclusive 256x192 clip.
# clip_segment: cdecl(pointer to four native signed shorts) -> accept.
# xor_segment: eax=x0 edx=y0 ebx=x1 ecx=y1 esi=buffer; preserves all registers.
.text
.globl _xor_line
_xor_line:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 subl $8,%esp
 movw 12(%ebp),%ax
 movw %ax,-16(%ebp)
 movw 16(%ebp),%ax
 movw %ax,-14(%ebp)
 movw 20(%ebp),%ax
 movw %ax,-12(%ebp)
 movw 24(%ebp),%ax
 movw %ax,-10(%ebp)
 leal -16(%ebp),%eax
 pushl %eax
 call clip_segment
 addl $4,%esp
 testl %eax,%eax
 je line_return
 movswl -16(%ebp),%eax
 movswl -14(%ebp),%edx
 movswl -12(%ebp),%ebx
 movswl -10(%ebp),%ecx
 movl 8(%ebp),%esi
 call xor_segment
line_return:
 addl $8,%esp
 popl %esi
 popl %ebx
 popl %ebp
 ret
code_xy:
 xorl %ecx,%ecx
 testw %ax,%ax
 jge code_right
 orl $1,%ecx
code_right:
 cmpw $255,%ax
 jle code_top
 orl $2,%ecx
code_top:
 testw %dx,%dx
 jge code_bottom
 orl $4,%ecx
code_bottom:
 cmpw $191,%dx
 jle code_done
 orl $8,%ecx
code_done:
 ret
.globl clip_segment
clip_segment:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $44,%esp
 movl 8(%ebp),%esi
 movswl (%esi),%eax
 movl %eax,-16(%ebp)
 movswl 2(%esi),%eax
 movl %eax,-20(%ebp)
 movswl 4(%esi),%eax
 movl %eax,-24(%ebp)
 movswl 6(%esi),%eax
 movl %eax,-28(%ebp)
 movl $8,-32(%ebp)
clip_again:
 movl -16(%ebp),%eax
 movl -20(%ebp),%edx
 call code_xy
 movl %ecx,-36(%ebp)
 movl -24(%ebp),%eax
 movl -28(%ebp),%edx
 call code_xy
 movl %ecx,%edi
 andl -36(%ebp),%edi
 jne clip_reject
 movl %ecx,%edi
 orl -36(%ebp),%edi
 je clip_accept
 cmpl $0,-32(%ebp)
 je clip_reject
 subl $1,-32(%ebp)
 movl -36(%ebp),%edi
 testl %edi,%edi
 jne clip_have
 movl %ecx,%edi
clip_have:
 movl -24(%ebp),%ebx
 subw -16(%ebp),%bx
 movswl %bx,%ebx
 movl %ebx,-44(%ebp)
 movl -28(%ebp),%edx
 subw -20(%ebp),%dx
 movswl %dx,%edx
 movl %edx,-48(%ebp)
 testl $8,%edi
 jne clip_bottom
 testl $4,%edi
 jne clip_top
 testl $2,%edi
 jne clip_right
 movl $0,-52(%ebp)
 jmp clip_vertical
clip_right:
 movl $255,-52(%ebp)
clip_vertical:
 testl %ebx,%ebx
 je clip_reject
 movl -52(%ebp),%eax
 subw -16(%ebp),%ax
 movswl %ax,%eax
 imull -48(%ebp),%eax
 call clip_divs
 addw -20(%ebp),%ax
 movswl %ax,%edx
 movl -52(%ebp),%eax
 jmp clip_store
clip_bottom:
 movl $191,-52(%ebp)
 jmp clip_horizontal
clip_top:
 movl $0,-52(%ebp)
clip_horizontal:
 movl -48(%ebp),%ebx
 testl %ebx,%ebx
 je clip_reject
 movl -52(%ebp),%eax
 subw -20(%ebp),%ax
 movswl %ax,%eax
 imull -44(%ebp),%eax
 call clip_divs
 addw -16(%ebp),%ax
 movswl %ax,%eax
 movl -52(%ebp),%edx
clip_store:
 cmpl $0,-36(%ebp)
 je clip_second
 movl %eax,-16(%ebp)
 movl %edx,-20(%ebp)
 jmp clip_again
clip_second:
 movl %eax,-24(%ebp)
 movl %edx,-28(%ebp)
 jmp clip_again
clip_accept:
 movw -16(%ebp),%ax
 movw %ax,(%esi)
 movw -20(%ebp),%ax
 movw %ax,2(%esi)
 movw -24(%ebp),%ax
 movw %ax,4(%esi)
 movw -28(%ebp),%ax
 movw %ax,6(%esi)
 movl $1,%eax
 jmp clip_return
clip_reject:
 xorl %eax,%eax
clip_return:
 addl $44,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
# Original 68000 DIVS.W leaves its dividend unchanged on quotient overflow.
clip_divs:
 movl %eax,%ecx
 cltd
 idivl %ebx
 cmpl $32767,%eax
 jg clip_div_overflow
 cmpl $-32768,%eax
 jl clip_div_overflow
 ret
clip_div_overflow:
 movl %ecx,%eax
 ret
.globl xor_segment
xor_segment:
 pushal
 cmpw %ax,%bx
 jge xor_ordered
 xchgl %eax,%ebx
 xchgl %edx,%ecx
xor_ordered:
 movl %edx,%edi
 shll $7,%edi
 addl %edi,%esi
 movl %eax,%edi
 shrl $2,%edi
 addl %edi,%edi
 addl %edi,%esi
 subw %ax,%bx
 subw %dx,%cx
 movl $128,%edx
 testw %cx,%cx
 jge xor_dy
 negw %cx
 negl %edx
xor_dy:
 andl $3,%eax
 movzwl xor_masks(,%eax,2),%eax
 cmpw %bx,%cx
 jg xor_steep
 movl %ebx,%edi
 shrw $1,%di
 movl %ebx,%ebp
xor_shallow_loop:
 xchgb %al,%ah
 xorw %ax,(%esi)
 xchgb %al,%ah
 subw %cx,%di
 jge xor_shallow_x
 addl %edx,%esi
 addw %bx,%di
xor_shallow_x:
 rorw $2,%ax
 jnc xor_shallow_next
 movw $0x80c0,%ax
 addl $2,%esi
xor_shallow_next:
 decw %bp
 cmpw $-1,%bp
 jne xor_shallow_loop
 jmp xor_done
xor_steep:
 movl %ecx,%edi
 shrw $1,%di
 movl %ecx,%ebp
xor_steep_loop:
 xchgb %al,%ah
 xorw %ax,(%esi)
 xchgb %al,%ah
 subw %bx,%di
 jge xor_steep_y
 rorw $2,%ax
 jnc xor_steep_adjust
 movw $0x80c0,%ax
 addl $2,%esi
xor_steep_adjust:
 addw %cx,%di
xor_steep_y:
 addl %edx,%esi
 decw %bp
 cmpw $-1,%bp
 jne xor_steep_loop
xor_done:
 popal
 ret
.globl _hash_screen
_hash_screen:
 pushl %esi
 movl 8(%esp),%esi
 movl $5381,%eax
 movl $32768,%ecx
hash_loop:
 movl %eax,%edx
 shll $5,%eax
 addl %edx,%eax
 movzbl (%esi),%edx
 incl %esi
 xorl %edx,%eax
 decl %ecx
 jne hash_loop
 popl %esi
 ret
.data
xor_masks: .word 0x80c0,0x2030,0x080c,0x0203
