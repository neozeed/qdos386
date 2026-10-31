# Original shell child spawning and fixed-slot lifetime rules.
.text
.globl asm_spawn_missile
asm_spawn_missile:
 cmpw $3,asm_mode
 jne spawn_return
 pushl %ebx
 pushl %esi
 pushl %edi
 pushl %ebp
 # Parent INWK is a real native pointer; stack local survives child allocation.
 pushl %esi
 cmpw $0,uni_active
 jne uni_spawn_allocate
 movl $3,%ebp
asm_spawn_find:
 imull $36,%ebp,%eax
 leal asm_vectors(%eax),%esi
 imull $272,%ebp,%eax
 leal asm_ships(%eax),%edi
 imull $448,%ebp,%eax
 leal asm_clouds(%eax),%ebx
 movb 31(%esi),%al
 andb $160,%al
 cmpb $160,%al
 jne asm_spawn_next
 cmpw $0,56(%ebx)
 jne asm_spawn_next
 jmp uni_spawn_ready
.globl uni_spawn_allocate
uni_spawn_allocate:
 movzwl asm_spawn_kind,%eax
 movl _models(,%eax,4),%eax
 movzwl 28(%eax),%eax
 call uni_allocate
 testl %eax,%eax
 js uni_spawn_full
 movl %eax,%ebp
 imull $36,%eax,%edx
 leal asm_vectors(%edx),%esi
 imull $272,%eax,%edx
 leal asm_ships(%edx),%edi
 imull $448,%eax,%edx
 leal asm_clouds(%edx),%ebx
uni_spawn_ready:
 movzwl asm_spawn_kind,%eax
 movl _models(,%eax,4),%eax
 movl %eax,(%edi)
 cmpb $0,asm_player+3
 je asm_spawn_enemy
 movzbl asm_controls+12,%eax
 pushl %eax
 movzbl asm_player+2,%eax
 cmpw $0,uni_active
 je uni_player_target
 incw %ax
uni_player_target:
 pushl %eax
 pushl %esi
 call _player_missile_init
 addl $12,%esp
 jmp asm_spawn_initialized
asm_spawn_enemy:
 pushl (%edi)
 pushl %esi
 pushl 8(%esp)
 call _spawn_child
 addl $12,%esp
asm_spawn_initialized:
 cmpw $8,asm_spawn_kind
 jne asm_spawn_no_tumble
 pushl $asm_laser+5
 pushl %esi
 call _cargo_tumble
 addl $8,%esp
 incl asm_debris
asm_spawn_no_tumble:
 cmpw $8,asm_spawn_kind
 je asm_count_spawned
 movw $0,14(%edi)
 movb $0,2(%ebx)
 cmpw $9,asm_spawn_kind
 jne asm_count_missile
 incl asm_pods
 jmp asm_count_spawned
asm_count_missile:
 incl asm_launches
 cmpb $0,asm_player+3
 jne sound_own_missile
 movl $6,%eax
 call asm_message
sound_own_missile:
 movl $48,%eax
 call asm_sound_event
asm_count_spawned:
 movl %esi,%ecx
 jmp asm_spawn_end
asm_spawn_next:
 incl %ebp
 cmpl $12,%ebp
 jb asm_spawn_find
uni_spawn_full:
 cmpw $8,asm_spawn_kind
 jne asm_spawn_failed
 pushl $1
 pushl $asm_laser+5
 call _elite_random
 addl $8,%esp
asm_spawn_failed:
 movl $0,%ecx
asm_spawn_end:
 addl $4,%esp
 popl %ebp
 popl %edi
 popl %esi
 popl %ebx
spawn_return:
 ret
.globl asm_abort_target
asm_abort_target:
 cmpw $0,uni_active
 jne abort_return
 pushal
 movw asm_slot,%ax
 cmpb asm_player+2,%al
 jne asm_abort_followers
 movb $255,asm_player+2
 movb $0,asm_player+1
 movl $5,%eax
 call asm_message
asm_abort_followers:
 movzwl asm_slot,%ebx
 addb %bl,%bl
 orb $128,%bl
 movl $asm_vectors,%esi
 movl $asm_ships,%edi
 movl $12,%ebp
asm_abort_loop:
 movl (%edi),%eax
 testl %eax,%eax
 je asm_abort_next
 cmpw $8,28(%eax)
 jne asm_abort_next
 cmpb %bl,32(%esi)
 jne asm_abort_next
 testb $160,31(%esi)
 jne asm_abort_next
 orb $128,31(%esi)
asm_abort_next:
 addl $272,%edi
 addl $36,%esi
 decl %ebp
 jne asm_abort_loop
 popal
abort_return:
 ret
.globl asm_drop_cargo
asm_drop_cargo:
 cmpw $3,asm_mode
 jne drop_return
 pushl %ebp
 pushl %ebx
 movl (%edi),%eax
 pushl $asm_laser+5
 movzwl 36(%eax),%edx
 pushl %edx
 movzwl 28(%eax),%eax
 pushl %eax
 call _debris_count
 addl $12,%esp
 movw %ax,%bp
 testw %bp,%bp
 je drop_done
 movw $8,asm_spawn_kind
drop_loop:
 call asm_spawn_missile
 decw %bp
 jne drop_loop
 movw $6,asm_spawn_kind
drop_done:
 popl %ebx
 popl %ebp
drop_return:
 ret
.globl asm_equipment_ouch
asm_equipment_ouch:
 pushl %ebx
 pushl %esi
 pushl %edi
 pushl %ebp
 movl $24,%eax
 call asm_sound_event
 movzwl asm_sound_carry,%eax
 pushl %eax
 movzbl asm_commander+7,%eax
 pushl %eax
 pushl $asm_laser+5
 pushl $asm_cargo
 call _equipment_damage
 addl $16,%esp
 testl %eax,%eax
 je ouch_done
 incl asm_losses
 decw %ax
 movl $asm_commodity_names,%esi
 cmpw $17,%ax
 jb ouch_name
 subw $17,%ax
 movl $asm_equipment_names,%esi
ouch_name:
 shlw $4,%ax
 movswl %ax,%eax
 addl %eax,%esi
 movl $asm_loss_text,%edi
 movl $16,%ecx
 cld
 rep
 movsb
 movb $22,asm_commander+7
 movb $3,asm_commander+8
ouch_done:
 popl %ebp
 popl %edi
 popl %esi
 popl %ebx
 ret
