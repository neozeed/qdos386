# RC6 motion.s. Native sign-magnitude rotation; no floating point.
# sign_add internal convention: ax,dx -> ax; clobbers dx; preserves others.
.text
.globl _own_rotate
_own_rotate:
 pushl %ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 20(%esp),%ebx
 movl $30,%eax
 movl $9,%edx
 call rotate_counter
 movl $29,%eax
 movl $21,%edx
 call rotate_counter
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
rotate_counter:
 movzbl (%ebx,%eax),%ecx
 movl %ecx,%ebp
 andl $127,%ecx
 je counter_done
 cmpl $127,%ecx
 je counter_fixed
 subb $1,(%ebx,%eax)
counter_fixed:
 andl $128,%ebp
 shll $8,%ebp
 leal 15(%ebx),%esi
 leal (%ebx,%edx),%edi
 pushl %ebx
 movl $2,%ebx
pair_loop:
 call mvs5_pair
 addl $2,%esi
 addl $2,%edi
 decl %ebx
 jns pair_loop
 popl %ebx
counter_done:
 ret
mvs5_pair:
 pushl %ebx
 movzwl (%esi),%ebx
 movzwl (%edi),%ecx
 movw %bx,%dx
 andw $32767,%dx
 shrw $9,%dx
 negw %dx
 addw %bx,%dx
 movw %cx,%ax
 andw $32767,%ax
 shrw $4,%ax
 pushl %edx
 movw %cx,%dx
 andw $32768,%dx
 orw %dx,%ax
 movl %ebp,%edx
 xorw %dx,%ax
 popl %edx
 call sign_add
 pushl %eax
 movw %cx,%dx
 andw $32767,%dx
 shrw $9,%dx
 negw %dx
 addw %cx,%dx
 movw %bx,%ax
 andw $32767,%ax
 shrw $4,%ax
 pushl %edx
 movw %bx,%dx
 andw $32768,%dx
 orw %dx,%ax
 xorw $32768,%ax
 movl %ebp,%edx
 xorw %dx,%ax
 popl %edx
 call sign_add
 movw %ax,(%edi)
 popl %eax
 movw %ax,(%esi)
 popl %ebx
 ret
.globl sign_add
sign_add:
 pushl %ecx
 movw %ax,%cx
 xorw %dx,%cx
 js add_different
 movw %ax,%cx
 andw $32768,%cx
 addw %dx,%ax
 orw %cx,%ax
 popl %ecx
 ret
add_different:
 movw %ax,%cx
 andw $32768,%cx
 andw $32767,%ax
 andw $32767,%dx
 subw %dx,%ax
 jnc add_sign
 negw %ax
 xorw $32768,%cx
add_sign:
 xorw %cx,%ax
 popl %ecx
 ret
.globl _motion_matrix
_motion_matrix:
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 16(%esp),%esi
 movl 20(%esp),%edi
 xorl %ecx,%ecx
matrix_loop:
 movzbl matrix_offsets(%ecx),%eax
 movzwl (%esi,%eax),%eax
 pushl %eax
 andl $32767,%eax
 xorl %edx,%edx
 movl $96,%ebx
 divl %ebx
 popl %edx
 testw %dx,%dx
 jns matrix_positive
 negw %ax
matrix_positive:
 # This public short[] result follows the native i386 C ABI (little endian).
 movw %ax,(%edi)
 addl $2,%edi
 incl %ecx
 cmpl $9,%ecx
 jb matrix_loop
 popl %edi
 popl %esi
 popl %ebx
 ret
.data
matrix_offsets: .byte 21,15,9,23,17,11,25,19,13
