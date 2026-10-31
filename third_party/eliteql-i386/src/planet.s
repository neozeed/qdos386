# RC6 planet.s. Full-width sign-magnitude MV40, including fractional borrow.
.text
.globl _move_planet
_move_planet:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $12,%esp
 movl 8(%ebp),%esi
 movl 12(%ebp),%edi
 call planet_load
 movzbl (%edi),%edx
 xorb $128,%dl
 call planet_multiply
 shrl $8,%eax
 movl %eax,-16(%ebp)
 addl $3,%esi
 call planet_load
 movl %eax,%edx
 movl -16(%ebp),%eax
 call planet_add
 movl %eax,-16(%ebp)
 movzbl 1(%edi),%edx
 call planet_multiply
 shrl $8,%eax
 movl %eax,-20(%ebp)
 addl $3,%esi
 call planet_load
 movl %eax,%edx
 movl -20(%ebp),%eax
 call planet_add
 movl %eax,-20(%ebp)
 call planet_store
 movl -20(%ebp),%eax
 xorl $0x800000,%eax
 movzbl 1(%edi),%edx
 call planet_multiply
 movl %eax,%ebx
 andl $0x80000000,%ebx
 movl -16(%ebp),%edx
 shll $8,%edx
 movzbl 16(%ebp),%ecx
 orl %ecx,%edx
 movl %eax,%ecx
 xorl %edx,%ecx
 js planet_y_sub
 addl %edx,%eax
 xorl %ebx,%eax
 shrl $8,%eax
 jmp planet_y_ready
planet_y_sub:
 andl $0x7fffffff,%eax
 andl $0x7fffffff,%edx
 subl %edx,%eax
 jnc planet_y_sign
 shrl $8,%eax
 negl %eax
 andl $0x7fffff,%eax
 xorl $0x80000000,%ebx
 shrl $8,%ebx
 orl %ebx,%eax
 jmp planet_y_ready
planet_y_sign:
 shrl $8,%eax
 shrl $8,%ebx
 orl %ebx,%eax
planet_y_ready:
 movl 8(%ebp),%esi
 addl $3,%esi
 call planet_store
 call planet_load
 movzbl (%edi),%edx
 call planet_multiply
 shrl $8,%eax
 movl %eax,-16(%ebp)
 movl 8(%ebp),%esi
 call planet_load
 movl %eax,%edx
 movl -16(%ebp),%eax
 call planet_add
 call planet_store
 addl $6,%esi
 call planet_load
 movl %eax,%edx
 movl $0x800000,%eax
 movb 2(%edi),%al
 call planet_add
 call planet_store
 addl $12,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
planet_load:
 movzbl 2(%esi),%eax
 shll $8,%eax
 movb 1(%esi),%al
 shll $8,%eax
 movb (%esi),%al
 ret
planet_store:
 movb %al,(%esi)
 shrl $8,%eax
 movb %al,1(%esi)
 shrl $8,%eax
 movb %al,2(%esi)
 ret
planet_multiply:
 movl %eax,%ebx
 shll $8,%ebx
 movl %edx,%ecx
 shll $24,%ecx
 xorl %ecx,%ebx
 andl $0x80000000,%ebx
 andl $127,%edx
 je planet_product_zero
 andl $0x7fffff,%eax
 imull %edx,%eax
 orl %ebx,%eax
 ret
planet_product_zero:
 xorl %eax,%eax
 ret
planet_add:
 movl %eax,%ebx
 andl $0x800000,%ebx
 movl %edx,%ecx
 andl $0x800000,%ecx
 andl $0x7fffff,%eax
 andl $0x7fffff,%edx
 cmpl %ebx,%ecx
 jne planet_add_opposite
 addl %edx,%eax
 andl $0x7fffff,%eax
 orl %ebx,%eax
 ret
planet_add_opposite:
 subl %eax,%edx
 jnc planet_coord_larger
 negl %edx
 movl %ebx,%ecx
planet_coord_larger:
 movl %edx,%eax
 orl %ecx,%eax
 ret
