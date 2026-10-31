# RC6 SCAN and MVEIT visibility, exact packed MODE 8 XOR drawing.
.text
.globl _scanner_flags
_scanner_flags:
 movl 4(%esp),%ecx
 testb $160,31(%ecx)
 jnz scanner_hide
 orb $16,31(%ecx)
 ret
scanner_hide:
 andb $239,31(%ecx)
 ret
.globl _scanner_project
_scanner_project:
 pushl %ebx
 pushl %esi
 movl 12(%esp),%esi
 movl 20(%esp),%ebx
 movb $0,3(%ebx)
 testb $128,16(%esp)
 jnz scanner_reject
 testb $16,31(%esi)
 jz scanner_reject
 movb 1(%esi),%al
 orb 4(%esi),%al
 orb 7(%esi),%al
 testb $192,%al
 jnz scanner_reject
 movzbl 1(%esi),%eax
 testb $128,2(%esi)
 jz scanner_xplus
 negl %eax
 jnz scanner_xplus
 movl $1,%eax
scanner_xplus:
 addl $123,%eax
 movb %al,(%ebx)
 movzbl 7(%esi),%eax
 shrl $2,%eax
 testb $128,8(%esi)
 jz scanner_zplus
 negl %eax
scanner_zplus:
 negl %eax
 addl $220,%eax
 movb %al,2(%ebx)
 movzbl 4(%esi),%edx
 shrl $1,%edx
 testb $128,5(%esi)
 jnz scanner_yminus
 negl %edx
scanner_yminus:
 addl %edx,%eax
 cmpl $194,%eax
 jge scanner_ymin
 movl $194,%eax
scanner_ymin:
 cmpl $246,%eax
 jle scanner_ymax
 movl $246,%eax
scanner_ymax:
 movb %al,1(%ebx)
 movb $1,3(%ebx)
 movl $1,%eax
 jmp scanner_project_done
scanner_reject:
 xorl %eax,%eax
scanner_project_done:
 popl %esi
 popl %ebx
 ret
.globl _scanner_xor
_scanner_xor:
 pushl %ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 20(%esp),%esi
 movl 24(%esp),%edi
 cmpb $0,3(%edi)
 je scanner_draw_done
 movzbl (%edi),%eax
 movzbl 1(%edi),%edx
 movzbl 2(%edi),%ebp
 movl %edx,%edi
 call scanner_pixel
 incl %eax
 call scanner_pixel
 incl %eax
 call scanner_pixel
 decl %edx
 call scanner_pixel
 decl %eax
 call scanner_pixel
 decl %eax
 call scanner_pixel
 addl $2,%eax
 cmpl %ebp,%edi
 je scanner_draw_done
 jl scanner_stick_down
 decl %edx
 decl %ebp
scanner_stick_up_loop:
 call scanner_pixel
 decl %edx
 cmpl %ebp,%edx
 jge scanner_stick_up_loop
 jmp scanner_draw_done
scanner_stick_down:
 leal 1(%edi),%edx
scanner_stick_down_loop:
 call scanner_pixel
 incl %edx
 cmpl %ebp,%edx
 jle scanner_stick_down_loop
scanner_draw_done:
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
# EAX=x EDX=y ESI=frame; ECX,EBX scratch.
scanner_pixel:
 movl %edx,%ecx
 shll $7,%ecx
 movl %eax,%ebx
 shrl $2,%ebx
 leal (%ecx,%ebx,2),%ecx
 movl %eax,%ebx
 andl $3,%ebx
 movw scanner_masks(,%ebx,2),%bx
 xorw %bx,(%esi,%ecx)
 ret
scanner_masks: .word 0xc080,0x3020,0x0c08,0x0302
.globl _scanner_reset
_scanner_reset:
 pushl %esi
 pushl %edi
 movl 12(%esp),%edi
 addl $24576,%edi
 movl $_dashboard_pixels,%esi
 movl $1792,%ecx
 cld
 rep
 movsl
 movl $scanner_cache,%edi
 movl $12,%ecx
 xorl %eax,%eax
 rep
 stosl
 popl %edi
 popl %esi
 ret
.globl _scanner_frame
_scanner_frame:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 movl $scanner_cache,%edi
 movl $12,%ebx
scanner_erase_loop:
 pushl %edi
 pushl 8(%ebp)
 call _scanner_xor
 addl $8,%esp
 movb $0,3(%edi)
 addl $4,%edi
 decl %ebx
 jnz scanner_erase_loop
 movl $scanner_cache,%edi
 movzwl 16(%ebp),%ebx
 decw %bx
 js scanner_frame_done
 movl 12(%ebp),%esi
scanner_frame_loop:
 pushl %esi
 call _scanner_flags
 addl $4,%esp
 pushl %edi
 pushl $1
 pushl %esi
 call _scanner_project
 addl $12,%esp
 pushl %edi
 pushl 8(%ebp)
 call _scanner_xor
 addl $8,%esp
 addl $36,%esi
 addl $4,%edi
 decw %bx
 cmpw $-1,%bx
 jne scanner_frame_loop
scanner_frame_done:
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.data
.align 2
scanner_cache: .space 48
