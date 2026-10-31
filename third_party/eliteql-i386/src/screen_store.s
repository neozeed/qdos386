# RC6 lossless backing-store RLE. Serialized count/value pairs remain BE.
.text
.globl _screen_pack
_screen_pack:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 8(%ebp),%esi
 movl 12(%ebp),%edi
 movl $16384,%ecx
 xorl %eax,%eax
pack_run:
 movw (%esi),%dx
 addl $2,%esi
 movl $1,%ebx
 decw %cx
 je pack_emit
pack_scan:
 cmpw (%esi),%dx
 jne pack_emit
 addl $2,%esi
 incw %bx
 decw %cx
 jne pack_scan
pack_emit:
 addl $4,%eax
 cmpl $0,12(%ebp)
 je pack_next
 cmpl 16(%ebp),%eax
 ja pack_overflow
 xchgb %bl,%bh
 movw %bx,(%edi)
 movw %dx,2(%edi)
 addl $4,%edi
pack_next:
 testw %cx,%cx
 jne pack_run
 jmp pack_done
pack_overflow:
 movl $-1,%eax
pack_done:
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.globl _screen_unpack
_screen_unpack:
 pushl %esi
 pushl %edi
 movl 12(%esp),%esi
 movl 16(%esp),%edi
 movl $16384,%ecx
unpack_run:
 movzwl (%esi),%eax
 xchgb %al,%ah
 addl $2,%esi
 testw %ax,%ax
 je unpack_bad
 cmpw %cx,%ax
 ja unpack_bad
 subw %ax,%cx
 movw (%esi),%dx
 addl $2,%esi
 decw %ax
unpack_copy:
 movw %dx,(%edi)
 addl $2,%edi
 decw %ax
 cmpw $-1,%ax
 jne unpack_copy
 testw %cx,%cx
 jne unpack_run
 xorl %eax,%eax
 jmp unpack_done
unpack_bad:
 movl $-1,%eax
unpack_done:
 popl %edi
 popl %esi
 ret
