# Native universe lifetime orchestration from shell.s:6057-6320.
# uni_state remains the original byte-serialized universe record.
.text
.globl uni_allocate
uni_allocate:
 pushl %ebx
 pushl %ecx
 pushl %edx
 pushl %esi
 pushl %edi
 pushl %ebp
 pushl %eax
 pushl %eax
 call _universe_cost
 addl $4,%esp
 pushl %eax
 pushl 4(%esp)
 pushl $uni_state
 call _universe_add
 addl $16,%esp
 testl %eax,%eax
 js uni_allocate_done
 subw $1,%ax
uni_allocate_done:
 popl %ebp
 popl %edi
 popl %esi
 popl %edx
 popl %ecx
 popl %ebx
 ret
.globl uni_encounter_plan
uni_encounter_plan:
 movl $0,_encounter_pool
 cmpw $0,uni_active
 je uni_plan_done
 pushal
 movl $uni_state,%esi
 movl $uni_plan,%edi
 movl $30,%ecx
 cld
 rep
 movsb
 movl $uni_plan,_encounter_pool
 movb $0,asm_enc_params+1
 movb $0,asm_enc_params+2
 movzwl uni_state+14,%edx
 xchgb %dl,%dh
 movl $2,%eax
uni_inventory:
 cmpw %dx,%ax
 jae uni_inventory_done
 cmpb $9,uni_state(%eax)
 jne uni_inventory_cop
 incb asm_enc_params+1
uni_inventory_cop:
 cmpb $2,uni_state(%eax)
 jne uni_inventory_next
 incb asm_enc_params+2
uni_inventory_next:
 incw %ax
 jmp uni_inventory
uni_inventory_done:
 movl $12,%eax
 subw %dx,%ax
 movb %al,asm_enc_params+7
 popal
uni_plan_done:
 ret
.globl uni_draw_entry
uni_draw_entry:
 movw asm_slot,%ax
 incw %ax
 movzwl uni_state+14,%edx
 xchgb %dl,%dh
 cmpw %dx,%ax
 jae uni_draw_finished
 cmpw $1,%ax
 jne uni_draw_present
 cmpb $129,uni_state+1
 jne uni_draw_present
 pushl $0
 pushl $asm_params
 pushl $asm_placeholder
 call _move_planet
 addl $12,%esp
 jmp uni_draw_next
uni_draw_present:
 movzwl asm_slot,%eax
 imull $272,%eax
 leal asm_ships(%eax),%edi
 movl $asm_vectors,%ecx
 movw asm_slot,%ax
 jmp uni_slot_ready
.globl uni_draw_complete
uni_draw_complete:
 movb 31(%esi),%al
 andw $160,%ax
 cmpw $160,%ax
 jne uni_draw_next
 call uni_remove_current
 jmp asm_draw_loop
uni_draw_next:
 incw asm_slot
 jmp asm_draw_loop
.globl uni_remove_current
uni_remove_current:
 pushal
 movw asm_slot,%ax
 movw %ax,uni_removed
 cmpb asm_player+2,%al
 jne uni_unlock_done
 movb $255,asm_player+2
 movb $0,asm_player+1
 movl $5,%eax
 call asm_message
uni_unlock_done:
 cmpw $0,asm_hidden
 jne uni_no_erase
 pushl %edi
 pushl _elite_screen
 call _render_cache
 addl $8,%esp
 movl asm_cloud,%eax
 movzwl 56(%eax),%edx
 pushl %edx
 addl $58,%eax
 pushl %eax
 pushl _elite_screen
 call _cloud_xor
 addl $12,%esp
uni_no_erase:
 movzwl uni_removed,%eax
 incl %eax
 pushl %eax
 pushl $uni_state
 call _universe_remove
 addl $8,%esp
 cmpw $0,uni_removed
 je uni_remove_station
 movw uni_removed,%ax
 movw %ax,uni_copy_index
uni_shift_blocks:
 movzwl uni_copy_index,%ebx
 leal 1(%ebx),%eax
 movzwl uni_state+14,%edx
 xchgb %dl,%dh
 cmpw %dx,%ax
 jae uni_clear_tail
 imull $36,%ebx,%eax
 leal asm_vectors(%eax),%edi
 leal 36(%edi),%esi
 movl $36,%ecx
 cld
 rep
 movsb
 imull $272,%ebx,%eax
 leal asm_ships(%eax),%edi
 leal 272(%edi),%esi
 movl $272,%ecx
 rep
 movsb
 imull $448,%ebx,%eax
 leal asm_clouds(%eax),%edi
 leal 448(%edi),%esi
 movl $448,%ecx
 rep
 movsb
 incw uni_copy_index
 jmp uni_shift_blocks
uni_clear_tail:
 imull $36,%ebx,%eax
 leal asm_vectors(%eax),%edi
 xorl %eax,%eax
 movl $36,%ecx
 cld
 rep
 stosb
 movb $160,-5(%edi)
 imull $272,%ebx,%eax
 movw $0,asm_ships+12(%eax)
 movw $1,asm_ships+14(%eax)
 imull $448,%ebx,%eax
 leal asm_clouds(%eax),%edi
 xorl %eax,%eax
 movl $448,%ecx
 rep
 stosb
 movl $1,%ebx
 movl $asm_vectors,%esi
uni_targets:
 movzwl uni_state+14,%edx
 xchgb %dl,%dh
 cmpw %dx,%bx
 jae uni_remove_done
 cmpb $8,uni_state(%ebx)
 jne uni_target_next
 movzwl uni_removed,%eax
 incl %eax
 pushl %eax
 movzbl 32(%esi),%eax
 pushl %eax
 call _universe_missile_target
 addl $8,%esp
 movb %al,32(%esi)
uni_target_next:
 addl $36,%esi
 incw %bx
 jmp uni_targets
uni_remove_station:
 movl $0,asm_station
 movl $asm_placeholder,%edi
 movl $36,%ecx
 xorl %eax,%eax
 cld
 rep
 stosb
 movb $6,asm_placeholder+5
 movb $160,asm_vectors+31
 movw $0,asm_ships+12
 movw $0,asm_clouds+56
uni_remove_done:
 popal
 ret
