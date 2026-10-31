# RC6 universe_list.s. count/heapUsed remain big-endian words in State30.
.text
.globl _universe_reset
_universe_reset:
 movl 4(%esp),%edx
 movl $29,%eax
ul_zero:
 movb $0,(%edx,%eax)
 decl %eax
 jns ul_zero
 movb $128,(%edx)
 movb $129,1(%edx)
 movw $512,14(%edx)
 ret
.globl _universe_cost
_universe_cost:
 movl 4(%esp),%eax
 andl $255,%eax
 cmpw $11,%ax
 ja ul_cost_zero
 movzbl ul_costs(%eax),%eax
 ret
ul_cost_zero:
 xorl %eax,%eax
 ret
.globl _universe_add
_universe_add:
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 16(%esp),%esi
 movl 20(%esp),%ebx
 movl 24(%esp),%edi
 cmpw $7,%bx
 je ul_station
 xorl %eax,%eax
ul_find:
 cmpb $0,(%esi,%eax)
 je ul_free
 incl %eax
 cmpl $12,%eax
 jb ul_find
 jmp ul_full
ul_free:
 movl %eax,%ecx
 incl %ecx
 imull $36,%ecx,%ecx
 movzwl 16(%esi),%edx
 xchgb %dl,%dh
 addl %edi,%edx
 addw %dx,%cx
 cmpw $1087,%cx
 ja ul_full
 movb %bl,(%esi,%eax)
 movl %edi,%ebx
 movb %bl,18(%esi,%eax)
 xchgb %dl,%dh
 movw %dx,16(%esi)
 movl %eax,%edx
 incl %edx
 xchgb %dl,%dh
 movw %dx,14(%esi)
 jmp ul_add_done
ul_station:
 movb $7,1(%esi)
 movl $1,%eax
 jmp ul_add_done
ul_full:
 movl $-1,%eax
ul_add_done:
 popl %edi
 popl %esi
 popl %ebx
 ret
.globl _universe_remove
_universe_remove:
 pushl %ebx
 movl 8(%esp),%edx
 movl 12(%esp),%eax
 andl $65535,%eax
 cmpw $1,%ax
 je ul_remove_station
 cmpw $2,%ax
 jb ul_remove_done
 movzwl 14(%edx),%ecx
 xchgb %cl,%ch
 cmpw %cx,%ax
 jae ul_remove_done
 movzwl 16(%edx),%ebx
 xchgb %bl,%bh
 pushl %ecx
 movzbl 18(%edx,%eax),%ecx
 subw %cx,%bx
 xchgb %bl,%bh
 movw %bx,16(%edx)
 popl %ecx
 decl %ecx
 movl %ecx,%ebx
 xchgb %bl,%bh
 movw %bx,14(%edx)
ul_shift:
 cmpw %cx,%ax
 jae ul_shift_end
 movb 1(%edx,%eax),%bl
 movb %bl,(%edx,%eax)
 movb 19(%edx,%eax),%bl
 movb %bl,18(%edx,%eax)
 incl %eax
 jmp ul_shift
ul_shift_end:
 movb $0,(%edx,%eax)
 movb $0,18(%edx,%eax)
 jmp ul_remove_done
ul_remove_station:
 movb $129,1(%edx)
ul_remove_done:
 popl %ebx
 ret
.globl _universe_missile_target
_universe_missile_target:
 movl 4(%esp),%eax
 andl $255,%eax
 testb $128,%al
 je ul_target_done
 movl %eax,%edx
 andl $127,%edx
 shrl $1,%edx
 cmpw 8(%esp),%dx
 jb ul_target_done
 je ul_target_dumb
 decw %dx
 addw %dx,%dx
 orw $128,%dx
 movw %dx,%ax
 ret
ul_target_dumb:
 xorl %eax,%eax
ul_target_done:
 ret
.data
ul_costs: .byte 0,61,77,93,85,153,153,0,81,65,49,25
