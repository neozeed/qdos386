# RC6 views.s, PLUT/PUS1. All 36 original bytes copied before transform.
.text
.globl _view_transform
_view_transform:
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 16(%esp),%edi
 movl 20(%esp),%esi
 movl $35,%ecx
view_copy:
 movb (%esi,%ecx),%al
 movb %al,(%edi,%ecx)
 decl %ecx
 jns view_copy
 movl 24(%esp),%eax
 testw %ax,%ax
 je view_done
 cmpw $1,%ax
 jne view_side
 xorb $128,2(%edi)
 xorb $128,8(%edi)
 leal 9(%edi),%esi
 movl $2,%ecx
view_rear_vectors:
 xorb $128,1(%esi)
 xorb $128,5(%esi)
 addl $6,%esi
 decl %ecx
 jns view_rear_vectors
 jmp view_done
view_side:
 movl $128,%ebx
 xorl %edx,%edx
 cmpw $2,%ax
 je view_side_signs
 xchgl %ebx,%edx
view_side_signs:
 movb (%edi),%al
 movb 6(%edi),%cl
 movb %cl,(%edi)
 movb %al,6(%edi)
 movb 1(%edi),%al
 movb 7(%edi),%cl
 movb %cl,1(%edi)
 movb %al,7(%edi)
 movb 2(%edi),%al
 xorb %bl,%al
 movb 8(%edi),%cl
 xorb %dl,%cl
 movb %cl,2(%edi)
 movb %al,8(%edi)
 leal 9(%edi),%esi
 movl $2,%edi
view_side_vectors:
 movb (%esi),%al
 movb 4(%esi),%cl
 movb %cl,(%esi)
 movb %al,4(%esi)
 movb 1(%esi),%al
 xorb %bl,%al
 movb 5(%esi),%cl
 xorb %dl,%cl
 movb %cl,1(%esi)
 movb %al,5(%esi)
 addl $6,%esi
 decl %edi
 jns view_side_vectors
view_done:
 popl %edi
 popl %esi
 popl %ebx
 ret
