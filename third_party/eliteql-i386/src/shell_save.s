# Original shell save_copy/save_checksum, with explicit cdecl data arguments.
# save_copy(buffer148, native descriptors[6] {pointer32,length16}, restore).
# Payload copies remain raw bytes, including commander CASH and its byte order.
.text
.globl _save_copy
_save_copy:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 12(%ebp),%ebx
 movl 8(%ebp),%edi
 addl $8,%edi
 movl $6,%edx
save_part:
 movl (%ebx),%esi
 movzwl 4(%ebx),%ecx
 addl $6,%ebx
save_byte:
 cmpw $0,16(%ebp)
 jne save_restore_byte
 movb (%esi),%al
 movb %al,(%edi)
 jmp save_byte_done
save_restore_byte:
 movb (%edi),%al
 movb %al,(%esi)
save_byte_done:
 incl %esi
 incl %edi
 decw %cx
 jnz save_byte
 decl %edx
 jnz save_part
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.globl _save_checksum
_save_checksum:
 pushl %esi
 movl 8(%esp),%esi
 movl $136,%ecx
 cmpl $0x31415145,(%esi)
 je save_checksum_ready
 movl $140,%ecx
save_checksum_ready:
 addl $8,%esi
 xorl %eax,%eax
save_sum:
 roll $1,%eax
 movzbl (%esi),%edx
 addl %edx,%eax
 incl %esi
 decl %ecx
 jnz save_sum
 popl %esi
 ret
