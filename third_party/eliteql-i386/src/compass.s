# RC6 SPS2/SP2/DOT calculations and three-pixel packed MODE 8 blips.
.text
.globl _compass_project
_compass_project:
 pushl %esi
 pushl %edi
 movl 12(%esp),%esi
 movl 16(%esp),%edi
 movzbl (%esi),%eax
 call compass_div10
 addw $193,%ax
 movb %al,(%edi)
 movzbl 1(%esi),%eax
 call compass_div10
 negw %ax
 addw $203,%ax
 movb %al,1(%edi)
 movb $240,2(%edi)
 cmpb $0,2(%esi)
 jns compass_front
 movb $255,2(%edi)
compass_front:
 movb $1,3(%edi)
 popl %edi
 popl %esi
 ret
compass_div10:
 movl %eax,%ecx
 andl $127,%eax
 pushl %ebx
 movl $10,%ebx
 xorl %edx,%edx
 divl %ebx
 popl %ebx
 testb %cl,%cl
 jns compass_positive
 negw %ax
compass_positive:
 ret
.globl _compass_xor
_compass_xor:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $8,%esp
 movl 8(%ebp),%esi
 movl 12(%ebp),%edi
 cmpb $0,3(%edi)
 je compass_xor_done
 movl $0,-16(%ebp)
 cmpb $240,2(%edi)
 jne compass_rows
 movl $1,-16(%ebp)
compass_rows:
 movzbl 1(%edi),%ebx
 subw -16(%ebp),%bx
 andl $65535,%ebx
 imull $128,%ebx,%ebx
 movl $2,-20(%ebp)
compass_pixels:
 movzbl (%edi),%eax
 addw -20(%ebp),%ax
 movl %eax,%ecx
 shrw $2,%ax
 addw %ax,%ax
 addw %bx,%ax
 movswl %ax,%eax
 andl $3,%ecx
 movw compass_masks(,%ecx,2),%dx
 xchgb %dl,%dh
 xorw %dx,(%esi,%eax)
 subl $1,-20(%ebp)
 jns compass_pixels
 subl $1,-16(%ebp)
 jns compass_rows
compass_xor_done:
 addl $8,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.globl _compass_frame
_compass_frame:
 pushl %ebp
 movl %esp,%ebp
 subl $4,%esp
 pushl 16(%ebp)
 pushl 8(%ebp)
 call _compass_xor
 addl $8,%esp
 leal -4(%ebp),%eax
 pushl %eax
 pushl 12(%ebp)
 cmpl $0,20(%ebp)
 je compass_use_planet
 call _tactics_vector
 jmp compass_vector_done
compass_use_planet:
 call _planet_vector
compass_vector_done:
 addl $8,%esp
 pushl 16(%ebp)
 leal -4(%ebp),%eax
 pushl %eax
 call _compass_project
 addl $8,%esp
 pushl 16(%ebp)
 pushl 8(%ebp)
 call _compass_xor
 addl $8,%esp
 leave
 ret
.data
compass_masks: .word 0x80c0,0x2030,0x080c,0x0203
