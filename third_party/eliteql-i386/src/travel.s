# RC6 travel.s: original galaxy generation, chart and hyperspace state.
# Byte arrays stay byte-identical. CASH access is explicitly big endian.
.text
.globl _galaxy_system
_galaxy_system:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $8,%esp
 movl 8(%ebp),%esi
 movl 16(%ebp),%edi
 movl $5,%ecx
 xorl %edx,%edx
gs_copy:
 movb (%esi,%edx),%al
 movb %al,(%edi,%edx)
 incl %edx
 decl %ecx
 jns gs_copy
 movl %edi,%esi
 movl 12(%ebp),%ebx
 andl $255,%ebx
 shll $2,%ebx
 decl %ebx
 js gs_data
gs_twists:
 call seed_twist
 decl %ebx
 jns gs_twists
gs_data:
 movzbl 1(%edi),%eax
 andl $7,%eax
 movzbl 2(%edi),%edx
 shrl $3,%edx
 andl $7,%edx
 movb %dl,7(%edi)
 cmpl $2,%edx
 jae gs_econ
 orl $2,%eax
gs_econ:
 movb %al,6(%edi)
 xorl $7,%eax
 movzbl 3(%edi),%ecx
 andl $3,%ecx
 addl %ecx,%eax
 incl %edx
 shrl $1,%edx
 addl %edx,%eax
 movb %al,8(%edi)
 movl (%edi),%eax
 movl %eax,-20(%ebp)
 movw 4(%edi),%ax
 movw %ax,-16(%ebp)
 leal -20(%ebp),%esi
 movl $2,%ebx
 testb $64,(%edi)
 je gs_name_start
 incl %ebx
gs_name_start:
 addl $9,%edi
gs_name:
 movzbl 5(%esi),%eax
 andl $31,%eax
 je gs_name_next
 addl %eax,%eax
 movb digrams(%eax),%cl
 movb %cl,(%edi)
 incl %edi
 movb digrams+1(%eax),%cl
 cmpb $63,%cl
 je gs_name_next
 movb %cl,(%edi)
 incl %edi
gs_name_next:
 call seed_twist
 decl %ebx
 jns gs_name
 movb $0,(%edi)
 addl $8,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
# Internal: esi -> six-byte LE-word seed; clobbers eax/edx only.
seed_twist:
 movzwl (%esi),%eax
 movzwl 2(%esi),%edx
 addw %dx,%ax
 movw %dx,(%esi)
 movzwl 4(%esi),%edx
 movw %dx,2(%esi)
 addw %dx,%ax
 movw %ax,4(%esi)
 ret
.globl _galaxy_distance
_galaxy_distance:
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 16(%esp),%esi
 movl 20(%esp),%edi
 movzbl 3(%esi),%eax
 movzbl 3(%edi),%edx
 subl %edx,%eax
 jns gd_x
 negl %eax
gd_x:
 imull %eax,%eax
 movl %eax,%ebx
 movzbl 1(%esi),%eax
 movzbl 1(%edi),%edx
 subl %edx,%eax
 jns gd_y
 negl %eax
gd_y:
 shrl $1,%eax
 imull %eax,%eax
 addw %ax,%bx
 andl $65535,%ebx
 xorl %eax,%eax
 movl $128,%ecx
gd_sqrt:
 orl %ecx,%eax
 movl %eax,%edx
 imull %edx,%edx
 cmpl %ebx,%edx
 jbe gd_bit_ok
 xorl %ecx,%eax
gd_bit_ok:
 shrl $1,%ecx
 jne gd_sqrt
 shll $2,%eax
 popl %edi
 popl %esi
 popl %ebx
 ret
.globl _travel_select
_travel_select:
 pushl %esi
 movl 8(%esp),%esi
 cmpb $0,11(%esi)
 jne ts_done
 movl 12(%esp),%eax
 movb %al,8(%esi)
 leal 40(%esi),%edx
 pushl %edx
 pushl %eax
 pushl %esi
 call _galaxy_system
 addl $12,%esp
ts_done:
 popl %esi
 ret
.globl _travel_init
_travel_init:
 pushl %esi
 movl 8(%esp),%esi
 movl $15,%ecx
ti_zero:
 movl $0,(%esi,%ecx,4)
 decl %ecx
 jns ti_zero
 movb $0x4a,(%esi)
 movb $0x5a,1(%esi)
 movb $0x48,2(%esi)
 movb $2,3(%esi)
 movb $0x53,4(%esi)
 movb $0xb7,5(%esi)
 movb $7,7(%esi)
 movb $70,9(%esi)
 movb $255,10(%esi)
 leal 16(%esi),%eax
 pushl %eax
 pushl $7
 pushl %esi
 call _galaxy_system
 addl $12,%esp
 pushl $7
 pushl %esi
 call _travel_select
 addl $8,%esp
 popl %esi
 ret
.globl _travel_start
_travel_start:
 pushl %esi
 movl 8(%esp),%esi
 movl $1,%eax
 cmpl $0,16(%esp)
 jne hs_exit
 movl $2,%eax
 cmpb $0,11(%esi)
 jne hs_exit
 cmpl $0,12(%esp)
 je hs_normal
 movl $5,%eax
 cmpb $0,10(%esi)
 je hs_exit
 movb $2,13(%esi)
 movb $0,10(%esi)
 jmp hs_begin
hs_normal:
 leal 40(%esi),%eax
 pushl %eax
 leal 16(%esi),%eax
 pushl %eax
 call _galaxy_distance
 addl $8,%esp
 testw %ax,%ax
 je hs_zero
 movzbl 9(%esi),%edx
 cmpw %dx,%ax
 ja hs_range
 movb $1,13(%esi)
hs_begin:
 movb $15,11(%esi)
 movb $15,12(%esi)
 xorl %eax,%eax
 jmp hs_exit
hs_zero:
 movl $3,%eax
 jmp hs_exit
hs_range:
 movl $4,%eax
hs_exit:
 movb %al,15(%esi)
 popl %esi
 ret
.globl _travel_tick
_travel_tick:
 movl 4(%esp),%edx
 xorl %eax,%eax
 cmpb $0,11(%edx)
 je ht_done
 subb $1,12(%edx)
 jne ht_done
 movb $5,12(%edx)
 subb $1,11(%edx)
 jne ht_done
 movb 13(%edx),%al
ht_done:
 ret
.globl _travel_arrive
_travel_arrive:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $24,%esp
 movl 8(%ebp),%edi
 movl 12(%ebp),%ebx
 movl 16(%ebp),%eax
 movb %al,14(%edi)
 cmpb $2,13(%edi)
 je ha_galaxy
 leal 40(%edi),%eax
 pushl %eax
 leal 16(%edi),%eax
 pushl %eax
 call _galaxy_distance
 addl $8,%esp
 subb %al,9(%edi)
 jmp ha_copy
ha_galaxy:
 movb $0,6(%ebx)
 addb $1,6(%edi)
 andb $7,6(%edi)
 movl $5,%ecx
ha_rotate:
 rolb $1,(%edi,%ecx)
 decl %ecx
 jns ha_rotate
 # Original nearest search uses the same seed order and first strict minimum.
 # Reuse chart_nearest, whose scoring and initial best=127 match Ghy exactly.
 pushl $96
 pushl $96
 pushl %edi
 call _chart_nearest
 addl $12,%esp
 movb $0,11(%edi)
 pushl %eax
 pushl %edi
 call _travel_select
 addl $8,%esp
ha_copy:
 movb 8(%edi),%al
 movb %al,7(%edi)
 xorl %ecx,%ecx
ha_copy_loop:
 movl 40(%edi,%ecx,4),%eax
 movl %eax,16(%edi,%ecx,4)
 incl %ecx
 cmpl $6,%ecx
 jb ha_copy_loop
 shrb $1,6(%ebx)
 movb $0,13(%edi)
 movb $0,11(%edi)
 movb $0,15(%edi)
 addl $24,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.globl _travel_refuel
_travel_refuel:
 pushl %ebx
 movl 8(%esp),%edx
 movl 12(%esp),%ecx
 movl $70,%eax
 subb 9(%edx),%al
 addw %ax,%ax
 movl (%ecx),%ebx
 xchgb %bl,%bh
 roll $16,%ebx
 xchgb %bl,%bh
 cmpl %ebx,%eax
 ja tr_fail
 subl %eax,%ebx
 xchgb %bl,%bh
 roll $16,%ebx
 xchgb %bl,%bh
 movl %ebx,(%ecx)
 movb $70,9(%edx)
 xorl %eax,%eax
 popl %ebx
 ret
tr_fail:
 movl $1,%eax
 popl %ebx
 ret
.globl _chart_move
_chart_move:
 movl 4(%esp),%eax
 andl $255,%eax
 movsbl 8(%esp),%edx
 movl %eax,%ecx
 addw %dx,%ax
 cmpw $255,%ax
 jbe chart_move_ok
 movw %cx,%ax
chart_move_ok:
 ret
.globl _chart_nearest
_chart_nearest:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 subl $24,%esp
 movl 8(%ebp),%esi
 movl (%esi),%eax
 movl %eax,-36(%ebp)
 movw 4(%esi),%ax
 movw %ax,-32(%ebp)
 leal -36(%ebp),%esi
 movl 12(%ebp),%eax
 andl $255,%eax
 movl %eax,-28(%ebp)
 movl 16(%ebp),%eax
 andl $255,%eax
 movl %eax,-24(%ebp)
 xorl %ebx,%ebx
 movl $127,%edi
 movl $0,-20(%ebp)
chart_nearest_loop:
 movzbl 3(%esi),%eax
 subl -28(%ebp),%eax
 jns chart_nearest_x
 negl %eax
chart_nearest_x:
 shrl $1,%eax
 movzbl 1(%esi),%edx
 subl -24(%ebp),%edx
 jns chart_nearest_y
 negl %edx
chart_nearest_y:
 shrl $1,%edx
 addl %edx,%eax
 cmpl %edi,%eax
 jae chart_nearest_next
 movl %eax,%edi
 movl %ebx,-20(%ebp)
chart_nearest_next:
 call seed_twist
 call seed_twist
 call seed_twist
 call seed_twist
 incl %ebx
 cmpl $256,%ebx
 jb chart_nearest_loop
 movl -20(%ebp),%eax
 addl $24,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.data
digrams: .ascii "ALLEXEGEZACEBISOUSESARMAINDIREA?ERATENBERALAVETIEDORQUANTEISRION"
