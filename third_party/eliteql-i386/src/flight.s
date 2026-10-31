# RC6 flight.s. Native i386 MVEIT parts 3-8, MVT6, MAD and adapters.
# Internal scalar convention: eax=d0, edx=d1, ebx=d2, ecx=d3;
# esi=coordinate/vector, edi=destination. Long-lived values are local spills.
.text
.globl _move_ship
_move_ship:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $32,%esp
 movl 8(%ebp),%eax
 movl %eax,-16(%ebp)
 movl 12(%ebp),%eax
 movl %eax,-20(%ebp)
 movl -16(%ebp),%esi
 movzbl 27(%esi),%eax
 shlb $2,%al
 andl $255,%eax
 movl %eax,-44(%ebp)
 leal 10(%esi),%edi
 movl $2,-40(%ebp)
forward_axis:
 movzbl (%edi),%eax
 movl %eax,%edx
 andl $127,%eax
 imull -44(%ebp),%eax
 shrw $8,%ax
 andw $128,%dx
 call translate_axis
 addl $3,%esi
 addl $2,%edi
 subl $1,-40(%ebp)
 jns forward_axis
 movl -16(%ebp),%esi
 movl -20(%ebp),%edi
 movb 27(%esi),%al
 addb 28(%esi),%al
 jns speed_positive
 movb $0,%al
speed_positive:
 cmpb 3(%edi),%al
 jb speed_ready
 movb 3(%edi),%al
speed_ready:
 movb %al,27(%esi)
 movb $0,28(%esi)
 call position_rotate
 movl -16(%ebp),%esi
 addl $6,%esi
 movl -20(%ebp),%edi
 movzbl 2(%edi),%eax
 movl $128,%edx
 call translate_axis
orientation_body:
 movl -16(%ebp),%esi
 addl $9,%esi
 movl $2,-40(%ebp)
vector_loop:
 call mvs4_vector
 addl $6,%esi
 subl $1,-40(%ebp)
 jns vector_loop
 pushl -16(%ebp)
 call _own_rotate
 addl $4,%esp
 addl $32,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
translate_axis:
 movzbl 2(%esi),%ebx
 shll $8,%ebx
 movb 1(%esi),%bl
 shll $8,%ebx
 movb (%esi),%bl
 movl %ebx,%ecx
 andl $0x800000,%ecx
 shll $16,%edx
 xorl %ecx,%edx
 jne translate_opposite
 addl %ebx,%eax
 orl %ecx,%eax
 jmp translate_store
translate_opposite:
 andl $0x7fffff,%ebx
 subl %eax,%ebx
 jnc translate_sign
 negl %ebx
 xorl $0x800000,%ecx
translate_sign:
 movl %ebx,%eax
 orl %ecx,%eax
translate_store:
 movb %al,(%esi)
 shrl $8,%eax
 movb %al,1(%esi)
 shrl $8,%eax
 movb %al,2(%esi)
 ret
coordinate_mag:
 movzwl (%esi),%eax
 ret
mvt6_native:
 movzwl (%esi),%ebx
 movb 2(%esi),%cl
 xorb %dl,%cl
 js mvt6_opposite
 addw %bx,%ax
 ret
mvt6_opposite:
 subw %ax,%bx
 jnc mvt6_coord_larger
 negw %bx
 movw %bx,%ax
 ret
mvt6_coord_larger:
 movw %bx,%ax
 xorb $128,%dl
 ret
coordinate_store:
 movb %al,(%esi)
 shrw $8,%ax
 movb %al,1(%esi)
 movb %dl,2(%esi)
 ret
position_rotate:
 movl -20(%ebp),%edi
 movzbl (%edi),%eax
 andl $127,%eax
 movl %eax,-24(%ebp)
 movzbl 1(%edi),%eax
 andl $127,%eax
 movl %eax,-28(%ebp)
 movl -16(%ebp),%esi
 call coordinate_mag
 imull -24(%ebp),%eax
 shrl $8,%eax
 movb (%edi),%dl
 andb $128,%dl
 xorb $128,%dl
 xorb 2(%esi),%dl
 addl $3,%esi
 call mvt6_native
 movw %ax,-32(%ebp)
 movb %dl,-36(%ebp)
 movzwl %ax,%eax
 imull -28(%ebp),%eax
 shrl $8,%eax
 movb 1(%edi),%dl
 andb $128,%dl
 xorb -36(%ebp),%dl
 movl -16(%ebp),%esi
 addl $6,%esi
 call mvt6_native
 movw %ax,%bx
 call coordinate_store
 movzwl %bx,%eax
 imull -28(%ebp),%eax
 shrl $8,%eax
 movb 1(%edi),%dl
 andb $128,%dl
 xorb -36(%ebp),%dl
 xorb 2(%esi),%dl
 js position_y_add
 movzwl -32(%ebp),%ebx
 andl $65535,%eax
 incl %eax
 subl %eax,%ebx
 jnc position_y_ready
 negl %ebx
 xorb $128,-36(%ebp)
 jmp position_y_ready
position_y_add:
 movzwl -32(%ebp),%ebx
 addw %ax,%bx
position_y_ready:
 movw %bx,-32(%ebp)
 movw %bx,%ax
 movb -36(%ebp),%dl
 movl -16(%ebp),%esi
 addl $3,%esi
 call coordinate_store
 movzwl -32(%ebp),%eax
 imull -24(%ebp),%eax
 shrl $8,%eax
 movb (%edi),%dl
 andb $128,%dl
 xorb -36(%ebp),%dl
 movl -16(%ebp),%esi
 call mvt6_native
 call coordinate_store
 ret
mvs4_vector:
 movl -20(%ebp),%edi
 movb (%edi),%al
 movb %al,-44(%ebp)
 movb 1(%esi),%al
 xorb $128,%al
 leal 2(%esi),%edi
 call mad_component
 movb 3(%esi),%al
 movl %esi,%edi
 call mad_component
 movl -20(%ebp),%edi
 movb 1(%edi),%al
 movb %al,-44(%ebp)
 movb 5(%esi),%al
 xorb $128,%al
 leal 2(%esi),%edi
 call mad_component
 movb 3(%esi),%al
 leal 4(%esi),%edi
 call mad_component
 ret
mad_component:
 movw %ax,%bx
 xorb -44(%ebp),%bl
 andl $128,%ebx
 shll $8,%ebx
 andl $127,%eax
 movzbl -44(%ebp),%edx
 andl $127,%edx
 je mad_zero
 imull %edx,%eax
 orw %bx,%ax
 jmp mad_add
mad_zero:
 xorl %eax,%eax
mad_add:
 movzwl (%edi),%edx
 call sign_add
 movw %ax,(%edi)
 shrw $8,%ax
 ret
.globl _motion_position
_motion_position:
 pushl %esi
 pushl %edi
 movl 12(%esp),%esi
 movl 16(%esp),%edi
 movl $2,%ecx
position_adapter_loop:
 movb 2(%esi),%dl
 andb $127,%dl
 jne position_adapter_reject
 movzwl (%esi),%eax
 cmpw $30000,%ax
 ja position_adapter_reject
 cmpb $0,2(%esi)
 jns position_adapter_positive
 negw %ax
position_adapter_positive:
 movw %ax,(%edi)
 addl $2,%edi
 addl $3,%esi
 decl %ecx
 jns position_adapter_loop
 movl $1,%eax
 jmp position_adapter_return
position_adapter_reject:
 xorl %eax,%eax
position_adapter_return:
 popl %edi
 popl %esi
 ret
.globl _rotate_body
_rotate_body:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $32,%esp
 movl 8(%ebp),%eax
 movl %eax,-16(%ebp)
 movl 12(%ebp),%eax
 movl %eax,-20(%ebp)
 jmp orientation_body
