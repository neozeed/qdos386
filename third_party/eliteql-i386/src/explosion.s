# RC6 DOEXP/PTCLS/EXS1. Original cloud byte layouts and RNG call order.
.text
.globl _cloud_advance
_cloud_advance:
 movl 4(%esp),%ecx
 pushl %ebx
 movl 12(%esp),%ebx
 movzbl 7(%ecx),%eax
 cmpl $32,%eax
 jae cloud_far
 shll $3,%eax
 movzbl 6(%ecx),%edx
 shrl $5,%edx
 andl $6,%edx
 orl %eax,%edx
 orl $1,%edx
 movzbl 1(%ebx),%eax
 addl $4,%eax
 jmp cloud_counter
cloud_far:
 movl $254,%edx
 movzbl 1(%ebx),%eax
 addl $5,%eax
cloud_counter:
 cmpl $255,%eax
 ja cloud_expired
 movb %al,1(%ebx)
 shll $8,%eax
 pushl %ecx
 movl %edx,%ecx
 xorl %edx,%edx
 divl %ecx
 popl %ecx
 cmpw $7168,%ax
 jae cloud_size_max
 shrw $5,%ax
 jmp cloud_size_store
cloud_size_max:
 movw $254,%ax
cloud_size_store:
 movb %al,(%ebx)
 andb $191,31(%ecx)
 movl $1,%eax
 popl %ebx
 ret
cloud_expired:
 orb $160,31(%ecx)
 xorl %eax,%eax
 popl %ebx
 ret
.globl _cloud_points
_cloud_points:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $32,%esp
 # -16 saved RNG1; -20 size; -24 vertex byte index; -28 particle;
 # -32 random shape; -36 y; -40 origin; -44 heap vertex pointer.
 movl 8(%ebp),%esi
 movl 12(%ebp),%edi
 movl 16(%ebp),%ebx
 movb 1(%ebx),%al
 movb %al,-16(%ebp)
 movzbl (%esi),%eax
 movl %eax,-20(%ebp)
 leal 7(%esi),%eax
 movl %eax,-44(%ebp)
 movl $10,-24(%ebp)
cloud_vertex:
 movzbl 2(%esi),%eax
 cmpl %eax,-24(%ebp)
 ja cloud_points_done
 movl $3,%ecx
cloud_seed_loop:
 movb 3(%esi,%ecx),%al
 xorb -24(%ebp),%al
 movb %al,(%ebx,%ecx)
 decl %ecx
 jns cloud_seed_loop
 movzbl 1(%esi),%eax
 testb %al,%al
 jns cloud_particle_count
 xorb $255,%al
cloud_particle_count:
 shrl $4,%eax
 orl $1,%eax
 movl %eax,-28(%ebp)
cloud_particle:
 call cloud_random
 movb %al,-32(%ebp)
 movl -44(%ebp),%ecx
 movzwl 2(%ecx),%eax
 call cloud_offset
 cmpw $191,%ax
 jae cloud_skip_y
 movb %al,-36(%ebp)
 movl -44(%ebp),%ecx
 movzwl (%ecx),%eax
 call cloud_offset
 cmpw $255,%ax
 ja cloud_next_particle
 movb %al,(%edi)
 movb -36(%ebp),%al
 movb %al,1(%edi)
 movb -32(%ebp),%al
 movb %al,2(%edi)
 movb $0,3(%edi)
 addl $4,%edi
 jmp cloud_next_particle
cloud_skip_y:
 call cloud_random
cloud_next_particle:
 decl -28(%ebp)
 jns cloud_particle
 addl $4,-24(%ebp)
 addl $4,-44(%ebp)
 jmp cloud_vertex
cloud_points_done:
 movb -16(%ebp),%al
 movb %al,1(%ebx)
 movb 20(%ebp),%al
 movb %al,3(%ebx)
 movl %edi,%eax
 subl 12(%ebp),%eax
 shrl $2,%eax
 addl $32,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
cloud_random:
 pushl $0
 pushl %ebx
 call _elite_random
 addl $8,%esp
 ret
cloud_offset:
 movw %ax,-40(%ebp)
 call cloud_random
 movl %eax,%edx
 andl $128,%edx
 movl %eax,%ecx
 shrw $8,%cx
 addb %al,%al
 orb %cl,%al
 movzbl %al,%eax
 imull -20(%ebp),%eax
 shrw $8,%ax
 testl %edx,%edx
 jnz cloud_offset_minus
 incw %ax
 addw -40(%ebp),%ax
 ret
cloud_offset_minus:
 negw %ax
 addw -40(%ebp),%ax
 ret
.globl _cloud_xor
_cloud_xor:
 pushl %ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 20(%esp),%esi
 movl 24(%esp),%edi
 movzwl 28(%esp),%ebp
 decw %bp
 js cloud_xor_done
cloud_xor_next:
 movzbl (%edi),%eax
 movzbl 1(%edi),%edx
 testb $7,%al
 jz cloud_dash_ready
 decw %ax
cloud_dash_ready:
 call cloud_dash
 cmpb $80,2(%edi)
 jae cloud_xor_advance
 testb $7,%dl
 jz cloud_second_below
 decw %dx
 jmp cloud_second
cloud_second_below:
 incw %dx
cloud_second:
 call cloud_dash
cloud_xor_advance:
 addl $4,%edi
 decw %bp
 cmpw $-1,%bp
 jne cloud_xor_next
cloud_xor_done:
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
cloud_dash:
 call cloud_pixel
 incw %ax
 call cloud_pixel
 decw %ax
 ret
cloud_pixel:
 movw %dx,%cx
 shlw $7,%cx
 movw %ax,%bx
 shrw $2,%bx
 addw %bx,%bx
 addw %bx,%cx
 movswl %cx,%ecx
 movl %eax,%ebx
 andl $3,%ebx
 movw cloud_masks(,%ebx,2),%bx
 xorw %bx,(%esi,%ecx)
 ret
cloud_masks: .word 0xc080,0x3020,0x0c08,0x0302
.globl _explosion_start
_explosion_start:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 8(%ebp),%esi
 movl 12(%ebp),%edi
 orb $32,31(%esi)
 andb $63,31(%esi)
 movb $0,28(%esi)
 movb $0,30(%esi)
 movb $18,1(%edi)
 movl 16(%ebp),%eax
 movb 32(%eax),%al
 movb %al,2(%edi)
 xorl %ebx,%ebx
cloud_start_seed:
 pushl $0
 pushl 20(%ebp)
 call _elite_random
 addl $8,%esp
 movb %al,3(%edi,%ebx)
 incl %ebx
 cmpl $4,%ebx
 jb cloud_start_seed
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.globl _explosion_frame
_explosion_frame:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 12(%ebp),%esi
 movl 16(%ebp),%edi
 movzwl 56(%edi),%eax
 pushl %eax
 leal 58(%edi),%eax
 pushl %eax
 pushl 8(%ebp)
 call _cloud_xor
 addl $12,%esp
 movw $0,56(%edi)
 movb 31(%esi),%al
 andb $160,%al
 cmpb $32,%al
 jne cloud_frame_done
 pushl %edi
 pushl %esi
 call _cloud_advance
 addl $8,%esp
 testl %eax,%eax
 jz cloud_frame_done
 cmpw $0,_effect_visible
 je cloud_frame_done
 movzbl 2(%edi),%ecx
 subw $6,%cx
 shrw $1,%cx
 xorl %edx,%edx
cloud_copy_vertices:
 movw _effect_vertices(%edx),%ax
 movw %ax,7(%edi,%edx)
 addl $2,%edx
 decw %cx
 jnz cloud_copy_vertices
 pushl $0
 pushl 20(%ebp)
 leal 58(%edi),%eax
 pushl %eax
 pushl %edi
 call _cloud_points
 addl $16,%esp
 movw %ax,56(%edi)
 orb $64,31(%esi)
 pushl %eax
 leal 58(%edi),%eax
 pushl %eax
 pushl 8(%ebp)
 call _cloud_xor
 addl $12,%esp
cloud_frame_done:
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
