# RC6 game renderer. Numeric model, matrix, position and cache words are native.
# Only packed pixel bytes are serialized. All arithmetic stays 80386 integer.
.text
.globl _render_ship
_render_ship:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $32,%esp
 # locals: -16 model, -20 matrix, -24 cursor, -28 index; -36 endpoints.
 movl 12(%ebp),%edi
 movw $0,_effect_visible
 pushl %edi
 pushl 8(%ebp)
 call _render_cache
 addl $8,%esp
 movw $0,12(%edi)
 cmpw $0,14(%edi)
 jne render_return
 movl (%edi),%ebx
 testl %ebx,%ebx
 jz render_return
 movl %ebx,-16(%ebp)
 movswl 8(%edi),%ecx
 cmpw $4,%cx
 jl render_return
 movw 4(%edi),%ax
 testw %ax,%ax
 jns origin_x_positive
 negw %ax
origin_x_positive:
 cmpw %cx,%ax
 jge render_return
 movw 6(%edi),%ax
 testw %ax,%ax
 jns origin_y_positive
 negw %ax
origin_y_positive:
 cmpw %cx,%ax
 jge render_return
 movl %ecx,%eax
 shrw $7,%ax
 cmpw $31,%ax
 jle distance_ready
 movw $31,%ax
distance_ready:
 movw %ax,distance_level
 movl %ecx,%eax
 shrw $8,%ax
 cmpw $16,%ax
 jl full_ship
 cmpw 6(%ebx),%ax
 jg distant_ship
full_ship:
 movl _live_matrix,%esi
 testl %esi,%esi
 jz render_return
 movl %esi,-20(%ebp)
 movw 4(%edi),%ax
 movw 6(%edi),%dx
 xorl %ebx,%ebx
camera_reduce:
 cmpw $255,%cx
 jle camera_reduced
 shrw $1,%cx
 testw %ax,%ax
 jns camera_reduce_x
 incw %ax
camera_reduce_x:
 sarw $1,%ax
 testw %dx,%dx
 jns camera_reduce_y
 incw %dx
camera_reduce_y:
 sarw $1,%dx
 incw %bx
 jmp camera_reduce
camera_reduced:
 movw %ax,camera_position
 movw %dx,camera_position+2
 movw %cx,camera_position+4
 movw %bx,camera_shift
 movswl camera_position+0,%eax
 movswl 0(%esi),%ecx
 imull %ecx,%eax
 movswl camera_position+2,%edx
 movswl 6(%esi),%ecx
 imull %ecx,%edx
 addl %edx,%eax
 movswl camera_position+4,%edx
 movswl 12(%esi),%ecx
 imull %ecx,%edx
 addl %edx,%eax
 sarl $8,%eax
 movw %ax,camera_local+0
 movswl camera_position+0,%eax
 movswl 2(%esi),%ecx
 imull %ecx,%eax
 movswl camera_position+2,%edx
 movswl 8(%esi),%ecx
 imull %ecx,%edx
 addl %edx,%eax
 movswl camera_position+4,%edx
 movswl 14(%esi),%ecx
 imull %ecx,%edx
 addl %edx,%eax
 sarl $8,%eax
 movw %ax,camera_local+2
 movswl camera_position+0,%eax
 movswl 4(%esi),%ecx
 imull %ecx,%eax
 movswl camera_position+2,%edx
 movswl 10(%esi),%ecx
 imull %ecx,%edx
 addl %edx,%eax
 movswl camera_position+4,%edx
 movswl 16(%esi),%ecx
 imull %ecx,%edx
 addl %edx,%eax
 sarl $8,%eax
 movw %ax,camera_local+4
 movl $0,visible_faces
 movl $0,visible_faces+4
 movl $0,visible_faces+8
 movl $0,visible_faces+12
 movb $1,visible_faces+15
 movl -16(%ebp),%ebx
 movl 18(%ebx),%esi
 movl $0,-28(%ebp)
face_loop:
 movl -28(%ebp),%eax
 cmpw 4(%ebx),%ax
 jge vertices_begin
 movw 6(%esi),%ax
 cmpw distance_level,%ax
 jl face_visible
 movswl (%esi),%eax
 movswl camera_local,%ecx
 imull %ecx,%eax
 movswl 2(%esi),%edx
 movswl camera_local+2,%ecx
 imull %ecx,%edx
 addl %edx,%eax
 movswl 4(%esi),%edx
 movswl camera_local+4,%ecx
 imull %ecx,%edx
 addl %edx,%eax
 movl 8(%esi),%edx
 movzwl camera_shift,%ecx
 shrl %cl,%edx
 addl %edx,%eax
 jns face_next
face_visible:
 movl -28(%ebp),%eax
 movb $1,visible_faces(%eax)
face_next:
 addl $12,%esi
 incl -28(%ebp)
 jmp face_loop
vertices_begin:
 xorl %eax,%eax
vertices_clear:
 movl $0,vertex_valid(%eax)
 addl $4,%eax
 cmpl $32,%eax
 jb vertices_clear
 movl 10(%ebx),%esi
 movl $0,-28(%ebp)
vertex_loop:
 movl -28(%ebp),%eax
 cmpw (%ebx),%ax
 jge edges_begin
 cmpw $0,_render_exploding
 jne vertex_project
 movw 6(%esi),%ax
 cmpw distance_level,%ax
 jl vertex_next
 movzbl 8(%esi),%eax
 cmpb $0,visible_faces(%eax)
 jne vertex_project
 movzbl 9(%esi),%eax
 cmpb $0,visible_faces(%eax)
 jne vertex_project
 movzbl 10(%esi),%eax
 cmpb $0,visible_faces(%eax)
 jne vertex_project
 movzbl 11(%esi),%eax
 cmpb $0,visible_faces(%eax)
 je vertex_next
vertex_project:
 incl _projected_total
 movl %esi,-24(%ebp)
 movl -20(%ebp),%ebx
 movswl 0(%esi),%eax
 movswl 0(%ebx),%ecx
 imull %ecx,%eax
 movswl 2(%esi),%edx
 movswl 2(%ebx),%ecx
 imull %ecx,%edx
 addl %edx,%eax
 movswl 4(%esi),%edx
 movswl 4(%ebx),%ecx
 imull %ecx,%edx
 addl %edx,%eax
 sarl $8,%eax
 addw 4(%edi),%ax
 movw %ax,vertex_world+0
 movswl 0(%esi),%eax
 movswl 6(%ebx),%ecx
 imull %ecx,%eax
 movswl 2(%esi),%edx
 movswl 8(%ebx),%ecx
 imull %ecx,%edx
 addl %edx,%eax
 movswl 4(%esi),%edx
 movswl 10(%ebx),%ecx
 imull %ecx,%edx
 addl %edx,%eax
 sarl $8,%eax
 addw 6(%edi),%ax
 movw %ax,vertex_world+2
 movswl 0(%esi),%eax
 movswl 12(%ebx),%ecx
 imull %ecx,%eax
 movswl 2(%esi),%edx
 movswl 14(%ebx),%ecx
 imull %ecx,%edx
 addl %edx,%eax
 movswl 4(%esi),%edx
 movswl 16(%ebx),%ecx
 imull %ecx,%edx
 addl %edx,%eax
 sarl $8,%eax
 addw 8(%edi),%ax
 movw %ax,vertex_world+4
 movswl vertex_world+4,%ecx
 cmpw $4,%cx
 jge vertex_z_ready
 movl $4,%ecx
vertex_z_ready:
 movswl vertex_world,%eax
 shll $8,%eax
 call render_divs
 addw $128,%ax
 call bound_projection
 movl -28(%ebp),%edx
 movw %ax,_effect_vertices(,%edx,4)
 movswl vertex_world+2,%eax
 shll $8,%eax
 call render_divs
 negw %ax
 addw $96,%ax
 call bound_projection
 movl -28(%ebp),%edx
 movw %ax,_effect_vertices+2(,%edx,4)
 movb $1,vertex_valid(%edx)
 movl -16(%ebp),%ebx
vertex_next:
 addl $12,%esi
 incl -28(%ebp)
 jmp vertex_loop
edges_begin:
 movw $1,_effect_visible
 cmpw $0,_render_exploding
 jne render_return
 movl _render_inwk,%esi
 testl %esi,%esi
 jz enemy_beam_done
 testb $64,31(%esi)
 jz enemy_beam_done
 andb $191,31(%esi)
 movzwl 34(%ebx),%edx
 movl %edx,%eax
 shrl $2,%eax
 cmpb $0,vertex_valid(%eax)
 je enemy_beam_done
 movw _effect_vertices(%edx),%ax
 cmpb $255,%al
 je enemy_beam_done
 cmpb $255,%ah
 je enemy_beam_done
 movw %ax,-36(%ebp)
 movw _effect_vertices+2(%edx),%ax
 movw %ax,-34(%ebp)
 xorl %eax,%eax
 testb $128,2(%esi)
 jz enemy_beam_side
 movl $255,%eax
enemy_beam_side:
 movw %ax,-32(%ebp)
 movzbl 6(%esi),%eax
 movw %ax,-30(%ebp)
 call render_append
 enemy_beam_done:
 movl -16(%ebp),%ebx
 movl 14(%ebx),%esi
 movl $0,-28(%ebp)
edge_loop:
 movl -28(%ebp),%eax
 cmpw 2(%ebx),%ax
 jge render_draw
 movw 12(%edi),%ax
 cmpw 8(%ebx),%ax
 jge render_draw
 movzbl 4(%esi),%eax
 cmpw distance_level,%ax
 jl edge_next
 movzbl 2(%esi),%eax
 cmpb $0,visible_faces(%eax)
 jne edge_vertices
 movzbl 3(%esi),%eax
 cmpb $0,visible_faces(%eax)
 je edge_next
edge_vertices:
 movzbl (%esi),%eax
 movzbl 1(%esi),%edx
 cmpb $0,vertex_valid(%eax)
 je edge_next
 cmpb $0,vertex_valid(%edx)
 je edge_next
 movl _effect_vertices(,%eax,4),%eax
 movl %eax,-36(%ebp)
 movl _effect_vertices(,%edx,4),%eax
 movl %eax,-32(%ebp)
 call render_append
edge_next:
 addl $6,%esi
 incl -28(%ebp)
 jmp edge_loop
render_draw:
 pushl %edi
 pushl 8(%ebp)
 call _render_cache
 addl $8,%esp
render_return:
 addl $32,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
# Appends one accepted line, retaining the original byte cache layout.
render_append:
 leal -36(%ebp),%eax
 pushl %eax
 call clip_segment
 addl $4,%esp
 testl %eax,%eax
 jz render_append_done
 movzwl 12(%edi),%edx
 leal 16(%edi,%edx,4),%edx
 movb -36(%ebp),%al
 movb %al,(%edx)
 movb -34(%ebp),%al
 movb %al,1(%edx)
 movb -32(%ebp),%al
 movb %al,2(%edx)
 movb -30(%ebp),%al
 movb %al,3(%edx)
 incw 12(%edi)
 incl _line_total
render_append_done:
 ret
bound_projection:
 cmpw $8191,%ax
 jle projection_low
 movw $8191,%ax
projection_low:
 cmpw $-8191,%ax
 jge projection_done
 movw $-8191,%ax
projection_done:
 ret
# DIVS.W leaves its dividend intact on quotient overflow. EDX scratch.
render_divs:
 pushl %ebx
 movl %eax,%ebx
 cltd
 idivl %ecx
 cmpl $32767,%eax
 jg render_divs_overflow
 cmpl $-32768,%eax
 jl render_divs_overflow
 popl %ebx
 ret
render_divs_overflow:
 movl %ebx,%eax
 popl %ebx
 ret
distant_ship:
 cmpw $0,_render_exploding
 jne render_return
 movswl 4(%edi),%eax
 shll $8,%eax
 call render_divs
 addw $128,%ax
 movw %ax,-36(%ebp)
 incw %ax
 movw %ax,-32(%ebp)
 movswl 6(%edi),%eax
 shll $8,%eax
 call render_divs
 negw %ax
 addw $96,%ax
 movw %ax,-34(%ebp)
 movw %ax,-30(%ebp)
 call render_append
 jmp render_draw
.globl _render_cache
_render_cache:
 pushl %ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 20(%esp),%esi
 movl 24(%esp),%edi
 movzwl 12(%edi),%ebp
 testl %ebp,%ebp
 jz cache_done
 addl $16,%edi
cache_loop:
 movzbl (%edi),%eax
 movzbl 1(%edi),%edx
 movzbl 2(%edi),%ebx
 movzbl 3(%edi),%ecx
 call xor_segment
 addl $4,%edi
 decl %ebp
 jnz cache_loop
cache_done:
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.data
.align 2
.globl _projected_total
.globl _line_total
.globl _effect_vertices
.globl _live_matrix
.globl _render_exploding
.globl _effect_visible
.globl _render_inwk
_projected_total: .long 0
_line_total: .long 0
visible_faces: .space 16
vertex_valid: .space 32
_effect_vertices: .space 128
camera_local: .space 6
vertex_world: .space 6
distance_level: .word 0
camera_position: .space 6
camera_shift: .word 0
_live_matrix: .long 0
_render_exploding: .word 0
_effect_visible: .word 0
_render_inwk: .long 0
