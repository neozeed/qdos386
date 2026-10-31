# RC6 TT24/TT25 and TT182..186. Numeric output shorts use native order.
.text
.globl _system_details
_system_details:
 pushl %ebx
 pushl %esi
 movl 12(%esp),%esi
 movl 16(%esp),%ecx
 movzbl 8(%esi),%eax
 shll $2,%eax
 movzbl 6(%esi),%edx
 addl %edx,%eax
 movzbl 7(%esi),%ebx
 addl %ebx,%eax
 incl %eax
 movw %ax,(%ecx)
 xorl $7,%edx
 addl $3,%edx
 imull %edx,%eax
 movzwl %ax,%eax
 addl $4,%ebx
 imull %ebx,%eax
 shlw $3,%ax
 movw %ax,2(%ecx)
 movzbl 5(%esi),%edx
 movl %edx,%eax
 andl $15,%eax
 addl $11,%eax
 shll $8,%eax
 movb 3(%esi),%al
 movw %ax,4(%ecx)
 movl %edx,%eax
 shrl $2,%eax
 andl $7,%eax
 movb %al,6(%ecx)
 movl %edx,%eax
 shrl $5,%eax
 movb %al,7(%ecx)
 movzbl 3(%esi),%eax
 xorb 1(%esi),%al
 andl $7,%eax
 movb %al,8(%ecx)
 andl $3,%edx
 addl %edx,%eax
 andl $7,%eax
 movb %al,9(%ecx)
 popl %esi
 popl %ebx
 ret
.globl _local_position
_local_position:
 pushl %ebx
 movl 8(%esp),%ecx
 movl 12(%esp),%edx
 movzbl 3(%ecx),%eax
 movzbl 3(%edx),%ebx
 subl %ebx,%eax
 cmpl $-19,%eax
 jl local_no
 cmpl $19,%eax
 jg local_no
 movl %eax,%ebx
 shrl $31,%ebx
 leal 104(%ebx,%eax,4),%eax
 movl 16(%esp),%ebx
 movw %ax,(%ebx)
 movzbl 1(%ecx),%eax
 movzbl 1(%edx),%ebx
 subl %ebx,%eax
 cmpl $-37,%eax
 jl local_no
 cmpl $37,%eax
 jg local_no
 movl %eax,%ebx
 shrl $31,%ebx
 leal 90(%ebx,%eax,2),%eax
 movl 16(%esp),%ebx
 movw %ax,2(%ebx)
 movl $1,%eax
 popl %ebx
 ret
local_no:
 xorl %eax,%eax
 popl %ebx
 ret
