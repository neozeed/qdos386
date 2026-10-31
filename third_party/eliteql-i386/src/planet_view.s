# RC6 PLANET / PROJ / DVID3B2 / CIRCLE. Native short geometry and cache.
.text
.globl _planet_geometry
_planet_geometry:
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 16(%esp),%esi
 movl 20(%esp),%edi
 movw $0,(%edi)
 movb 8(%esi),%al
 cmpb $48,%al
 jae pg_done
 orb 7(%esi),%al
 jz pg_done
 leal 6(%esi),%edx
 call pg_load
 movl %eax,%ebx
 movl %esi,%edx
 call pg_load
 call pg_divide
 cmpl $1024,%eax
 jae pg_done
 testb $128,2(%esi)
 jz pg_x
 negw %ax
pg_x:
 addw $128,%ax
 movw %ax,2(%edi)
 leal 3(%esi),%edx
 call pg_load
 call pg_divide
 cmpl $1024,%eax
 jae pg_done
 testb $128,5(%esi)
 jnz pg_y
 negw %ax
pg_y:
 addw $96,%ax
 movw %ax,4(%edi)
 movl $24576,%eax
 call pg_divide
 cmpl $256,%eax
 jb pg_radius
 movw $248,%ax
pg_radius:
 movw %ax,6(%edi)
 movw $1,(%edi)
pg_done:
 popl %edi
 popl %esi
 popl %ebx
 ret
pg_load:
 movzbl 2(%edx),%eax
 andl $127,%eax
 shll $8,%eax
 movb 1(%edx),%al
 shll $8,%eax
 movb (%edx),%al
 ret
# EAX numerator, EBX denominator. EDX/ECX scratch. Original count semantics.
pg_divide:
 orl $1,%eax
 xorl %ecx,%ecx
pg_numerator:
 cmpl $0x400000,%eax
 jae pg_denominator
 shll $1,%eax
 incl %ecx
 jmp pg_numerator
pg_denominator:
 movl %ebx,%edx
pg_den_loop:
 shll $1,%edx
 decl %ecx
 testl $0x800000,%edx
 jz pg_den_loop
 shrl $16,%eax
 andl $255,%eax
 shrl $16,%edx
 andl $255,%edx
 shll $8,%eax
 pushl %ecx
 movl %edx,%ecx
 xorl %edx,%edx
 divl %ecx
 popl %ecx
 andl $255,%eax
 testl %ecx,%ecx
 js pg_shift_left
 cmpl $32,%ecx
 jae pg_shift_zero
 shrl %cl,%eax
 ret
pg_shift_left:
 negl %ecx
 cmpl $32,%ecx
 jae pg_shift_zero
 shll %cl,%eax
 ret
pg_shift_zero:
 xorl %eax,%eax
 ret
.globl _circle_xy
_circle_xy:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $28,%esp
 movl 8(%ebp),%esi
 movl 12(%ebp),%edi
 movw $0,12(%edi)
 movw 16(%ebp),%ax
 movw %ax,-16(%ebp)
 movw 20(%ebp),%ax
 movw %ax,-18(%ebp)
 movw 24(%ebp),%ax
 movw %ax,-20(%ebp)
 movw 28(%ebp),%ax
 movw %ax,-22(%ebp)
 jmp pv_step_ready
.globl _circle_ring
_circle_ring:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $28,%esp
 movl 8(%ebp),%esi
 movl 12(%ebp),%edi
 movw $0,12(%edi)
 movw $128,-16(%ebp)
 movw $96,-18(%ebp)
 movw 16(%ebp),%ax
 movw %ax,-20(%ebp)
 movw 20(%ebp),%ax
 movw %ax,-22(%ebp)
 jmp pv_step_ready
.globl _planet_frame
_planet_frame:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $28,%esp
 movl 8(%ebp),%esi
 movl 12(%ebp),%edi
 pushl %edi
 pushl %esi
 call _render_cache
 addl $8,%esp
 movw $0,12(%edi)
 leal -40(%ebp),%eax
 pushl %eax
 pushl 16(%ebp)
 call _planet_geometry
 addl $8,%esp
 cmpw $0,-40(%ebp)
 je pv_done
 movw -38(%ebp),%ax
 movw %ax,-16(%ebp)
 movw -36(%ebp),%ax
 movw %ax,-18(%ebp)
 movw -34(%ebp),%ax
 movw %ax,-20(%ebp)
 movw $8,-22(%ebp)
 cmpw $9,%ax
 jb pv_step_ready
 movw $4,-22(%ebp)
pv_step_ready:
 movw $0,-24(%ebp)
pv_point:
 movzwl -24(%ebp),%eax
 call pv_sine
 cmpw $33,-24(%ebp)
 jb pv_x
 negw %ax
pv_x:
 addw -16(%ebp),%ax
 movw %ax,-28(%ebp)
 movzwl -24(%ebp),%eax
 addw $16,%ax
 call pv_sine
 movw -24(%ebp),%dx
 addw $16,%dx
 andw $63,%dx
 cmpw $33,%dx
 jb pv_y
 negw %ax
pv_y:
 addw -18(%ebp),%ax
 movw %ax,-26(%ebp)
 cmpw $0,-24(%ebp)
 je pv_next
 # Previous point -32,-30; current -28,-26. Clip a copy to retain points.
 movl -32(%ebp),%eax
 movl %eax,-40(%ebp)
 movl -28(%ebp),%eax
 movl %eax,-36(%ebp)
 leal -40(%ebp),%eax
 pushl %eax
 call clip_segment
 addl $4,%esp
 testl %eax,%eax
 jz pv_next
 movzwl 12(%edi),%ecx
 leal 16(%edi,%ecx,4),%ecx
 movzbl -40(%ebp),%eax
 movb %al,(%ecx)
 movzbl -38(%ebp),%edx
 movb %dl,1(%ecx)
 movzbl -36(%ebp),%ebx
 movb %bl,2(%ecx)
 movzbl -34(%ebp),%eax
 movb %al,3(%ecx)
 movl %eax,%ecx
 movzbl -40(%ebp),%eax
 incw 12(%edi)
 call xor_segment
pv_next:
 movl -28(%ebp),%eax
 movl %eax,-32(%ebp)
 movw -22(%ebp),%ax
 addw %ax,-24(%ebp)
 cmpw $65,-24(%ebp)
 jb pv_point
pv_done:
 addl $28,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
pv_sine:
 andl $31,%eax
 movzbl planet_sine(%eax),%eax
 movzwl -20(%ebp),%edx
 imull %edx,%eax
 shrw $8,%ax
 ret
.globl _planet_altitude
_planet_altitude:
 pushl %ebx
 movl 8(%esp),%ecx
 movb 2(%ecx),%al
 orb 5(%ecx),%al
 orb 8(%ecx),%al
 testb $127,%al
 jnz pa_far
 xorl %edx,%edx
 movl $3,%ebx
pa_sum:
 movzbl 1(%ecx),%eax
 imull %eax,%eax
 shrw $8,%ax
 addw %ax,%dx
 cmpw $256,%dx
 jae pa_far
 addl $3,%ecx
 decl %ebx
 jnz pa_sum
 subw $37,%dx
 js pa_inside
 shlw $8,%dx
 movzbl 12(%esp),%eax
 orw %ax,%dx
 xorl %eax,%eax
 movl $1,%ecx
pa_root:
 cmpw %cx,%dx
 jb pa_root_done
 subw %cx,%dx
 addw $2,%cx
 incw %ax
 jmp pa_root
pa_root_done:
 testw %ax,%ax
 jnz pa_return
 movl $256,%eax
 jmp pa_return
pa_far:
 movl $255,%eax
 jmp pa_return
pa_inside:
 movl $511,%eax
pa_return:
 popl %ebx
 ret
.data
planet_sine: .byte 0,25,50,74,98,121,142,162,181,198,213,226,237,245,251,255,255,255,251,245,237,226,213,198,181,162,142,121,98,74,50,25
