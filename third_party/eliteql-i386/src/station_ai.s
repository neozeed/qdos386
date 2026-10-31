# RC6 station_ai.s. Original station launch gate and sign-magnitude geometry.
.text
.globl _station_police
_station_police:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 8(%ebp),%edi
 movl 12(%ebp),%esi
 pushl $1
 pushl 16(%ebp)
 call _elite_random
 addl $8,%esp
 cmpb $140,%al
 jb police_no
 cmpl $3,20(%ebp)
 jae police_no
 movl $35,%ecx
police_copy:
 movb (%esi,%ecx),%al
 movb %al,(%edi,%ecx)
 decl %ecx
 jns police_copy
 movb $32,27(%edi)
 movb $225,32(%edi)
 andb $254,29(%edi)
 leal 10(%edi),%esi
 movl $2,%ecx
police_axis:
 pushl %ecx
 movzbl (%esi),%eax
 movl %eax,%ebx
 andl $128,%ebx
 shll $16,%ebx
 andl $127,%eax
 addw %ax,%ax
 movzbl 2(%edi),%edx
 shll $8,%edx
 movb 1(%edi),%dl
 shll $8,%edx
 movb (%edi),%dl
 movl %edx,%ecx
 andl $0x800000,%ecx
 andl $0x7fffff,%edx
 cmpl %ebx,%ecx
 jne police_sub
 addl %edx,%eax
 andl $0xffffff,%eax
 orl %ecx,%eax
 jmp police_store
police_sub:
 subl %eax,%edx
 jnc police_coord_sign
 negl %edx
 xorl $0x800000,%ecx
police_coord_sign:
 movl %edx,%eax
 orl %ecx,%eax
police_store:
 movb %al,(%edi)
 shrl $8,%eax
 movb %al,1(%edi)
 shrl $8,%eax
 movb %al,2(%edi)
 addl $3,%edi
 addl $2,%esi
 popl %ecx
 decl %ecx
 jns police_axis
 movl $1,%eax
 jmp police_return
police_no:
 xorl %eax,%eax
police_return:
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
