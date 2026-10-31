# RC6 navigation.s: SPS1/SPS3 and original mode-4-compatible bitplane icons.
# The shell selects MODE 8; original pixel masks below are preserved verbatim.
.text
.globl _planet_vector
_planet_vector:
 pushl %ebp
 movl %esp,%ebp
 pushl %esi
 pushl %edi
 subl $12,%esp
 movl 8(%ebp),%esi
 leal -20(%ebp),%edi
 movl $2,%ecx
planet_axis:
 movb 1(%esi),%al
 movb %al,(%edi)
 movb 2(%esi),%al
 andb $127,%al
 movb %al,1(%edi)
 movb 2(%esi),%al
 andb $128,%al
 movb %al,2(%edi)
 addl $3,%esi
 addl $3,%edi
 decl %ecx
 jns planet_axis
 pushl 12(%ebp)
 leal -20(%ebp),%eax
 pushl %eax
 call _tactics_vector
 addl $8,%esp
 addl $12,%esp
 popl %edi
 popl %esi
 popl %ebp
 ret
.globl _missile_icons
_missile_icons:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $12,%esp
 movl $4,-16(%ebp)
icons_slot:
 xorl %edx,%edx
 movl 12(%ebp),%esi
 movzbl (%esi),%eax
 cmpw -16(%ebp),%ax
 jb icons_shape
 movl $9,%edx
 jne icons_shape
 cmpb $0,1(%esi)
 je icons_lock
 movl $13,%edx
icons_lock:
 cmpb $0,2(%esi)
 js icons_shape
 movl $17,%edx
icons_shape:
 testl %edx,%edx
 jne icons_have_shape
 movl $4,%edx
icons_have_shape:
 leal icon_bits(%edx),%edi
 movl -16(%ebp),%eax
 shll $3,%eax
 negl %eax
 addl $48,%eax
 shrl $1,%eax
 movl 8(%ebp),%esi
 addl %eax,%esi
 addl $31360,%esi
 movl $4,-20(%ebp)
icons_row:
 xorl %ebx,%ebx
 xorl %edx,%edx
 movb (%edi),%al
 decl %edi
 movl $7,%ecx
icons_pixels:
 shlw $2,%bx
 shlw $2,%dx
 addb %al,%al
 jnc icons_black
 orw $2,%bx
 orw $3,%dx
icons_black:
 decl %ecx
 jns icons_pixels
 andb $85,(%esi)
 orb %bh,(%esi)
 movb %dh,1(%esi)
 andb $85,2(%esi)
 orb %bl,2(%esi)
 movb %dl,3(%esi)
 subl $128,%esi
 subl $1,-20(%ebp)
 jns icons_row
 subl $1,-16(%ebp)
 jne icons_slot
 addl $12,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.data
icon_bits: .byte 0,0,0,0,0,252,252,252,252,252,132,180,132,252,196,236,236,252
