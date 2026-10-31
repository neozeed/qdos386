# Original shell world and presentation sequencing; native register allocation.
.text
.globl asm_restore_view
asm_restore_view:
 movw $0,asm_hidden
 movl $3,%eax
 cmpw $0,asm_dust_valid
 jne restore_stars
 xorl %eax,%eax
 movw $1,asm_dust_valid
restore_stars:
 pushl %eax
 call asm_dust_run
 addl $4,%esp
 pushl $asm_planet_cache
 pushl _elite_screen
 call _render_cache
 addl $8,%esp
 pushl %ebx
 pushl %ebp
 movl $asm_ships,%edi
 movl $asm_clouds,%ebx
 movl $12,%ebp
restore_loop:
 pushl %edi
 pushl _elite_screen
 call _render_cache
 addl $8,%esp
 movzwl 56(%ebx),%eax
 pushl %eax
 leal 58(%ebx),%eax
 pushl %eax
 pushl _elite_screen
 call _cloud_xor
 addl $12,%esp
 addl $272,%edi
 addl $448,%ebx
 decl %ebp
 jne restore_loop
 popl %ebp
 popl %ebx
 cmpb $0,asm_sight
 je restore_laser
 pushl _elite_screen
 call _sight_xor
 addl $4,%esp
restore_laser:
 cmpb $0,asm_laser+9
 je restore_done
 pushl $asm_laser
 pushl _elite_screen
 call _laser_xor
 addl $8,%esp
restore_done:
 ret
.globl asm_warp
asm_warp:
 cmpw $0,asm_world
 je warp_return
 movw $2,asm_warp_result
 movw $32,asm_warp_timer
 cmpb $0,asm_travel+11
 jne warp_done
 xorl %ebx,%ebx
 cmpl $0,asm_station
 jne warp_block
 pushl %esi
 pushl %edi
 pushl %ebp
 movl $asm_vectors,%esi
 movl $asm_ships,%edi
 movl $12,%ebp
warp_scan:
 movb 31(%esi),%al
 andb $160,%al
 cmpb $160,%al
 je warp_next
 movl (%edi),%eax
 cmpw $9,28(%eax)
 jb warp_found
 cmpw $11,28(%eax)
 ja warp_found
warp_next:
 addl $36,%esi
 addl $272,%edi
 decl %ebp
 jne warp_scan
 jmp warp_scanned
warp_found:
 movl $1,%ebx
warp_scanned:
 popl %ebp
 popl %edi
 popl %esi
 jmp warp_try
warp_block:
 movl $1,%ebx
warp_try:
 pushl %ebx
 pushl $asm_placeholder
 pushl $asm_planet
 call _warp_world
 addl $12,%esp
 testw %ax,%ax
 je warp_done
 movw $1,asm_warp_result
 incl asm_warps
 movb $1,asm_mcnt
 movb $0,asm_enc_params+5
 pushl $3
 call asm_dust_run
 addl $4,%esp
 pushl $4
 call asm_dust_run
 addl $4,%esp
 jmp warp_return
warp_done:
 cmpw $2,asm_warp_result
 jne warp_return
 movl $40,%eax
 call asm_sound_event
warp_return:
 ret
.globl asm_tunnel
asm_tunnel:
 pushal
 movzwl %ax,%ebp
 movl $48,%eax
 cmpw $4,%bp
 jne tunnel_sound
 movl $56,%eax
tunnel_sound:
 call asm_sound_event
 call asm_clear_view
 movw $0,asm_dust_valid
 xorl %ebx,%ebx
tunnel_group:
 movl %ebx,%esi
 andw $7,%si
 addw $8,%si
 movw %bx,asm_tunnel_phase
 incw asm_tunnel_phase
tunnel_ring:
 pushl %ebp
 pushl %esi
 pushl $asm_planet_cache
 pushl _elite_screen
 call _circle_ring
 addl $16,%esp
 addw %si,%si
 cmpw $160,%si
 jb tunnel_ring
 pushl $2
 call _platform_yield50
 addl $4,%esp
 call asm_sound_poll
 incw %bx
 cmpw $16,%bx
 jb tunnel_group
 movw $0,asm_tunnel_phase
 movw $0,asm_planet_cache+12
 popal
 ret
.globl asm_encounters
asm_encounters:
 cmpw $0,asm_ambient
 je encounter_return
 cmpw $3,asm_mode
 jne encounter_return
 movb $0,asm_enc_params
 movb $0,asm_enc_params+1
 movb $0,asm_enc_params+2
 movb $0,asm_enc_params+7
 movb asm_travel+23,%al
 movb %al,asm_enc_params+3
 movb asm_commander+6,%al
 movb %al,asm_enc_params+4
 movb asm_dust+166,%al
 movb %al,asm_enc_params+6
 movl $asm_vectors,%esi
 movl $asm_ships,%edi
 movl $asm_clouds,%ebp
 movl $12,%ecx
encounter_inventory:
 movb 31(%esi),%al
 andb $160,%al
 cmpb $160,%al
 jne encounter_occupied
 cmpw $0,56(%ebp)
 jne encounter_inventory_next
 incb asm_enc_params+7
 jmp encounter_inventory_next
encounter_occupied:
 testb %al,%al
 jne encounter_inventory_next
 movl (%edi),%eax
 movw 28(%eax),%ax
 cmpw $7,%ax
 jne encounter_asteroid
 movb $1,asm_enc_params
 jmp encounter_inventory_next
encounter_asteroid:
 cmpw $9,%ax
 jne encounter_cop
 incb asm_enc_params+1
encounter_cop:
 cmpw $2,%ax
 jne encounter_inventory_next
 incb asm_enc_params+2
encounter_inventory_next:
 addl $36,%esi
 addl $272,%edi
 addl $448,%ebp
 decl %ecx
 jne encounter_inventory
 call uni_encounter_plan
 pushl $asm_cargo
 pushl $asm_enc_params
 pushl $asm_laser+5
 pushl $asm_enc_output
 call _encounter_step
 addl $16,%esp
 movb %al,asm_dust+166
 movl $0,_encounter_pool
.globl encounter_emit
encounter_emit:
 pushl %ebx
 movl $asm_enc_output+2,%ebx
 movzwl asm_enc_output,%eax
 xchgb %al,%ah
 decw %ax
 js encounter_empty
 pushl %eax
encounter_insert:
 cmpw $0,uni_active
 jne uni_encounter_allocate
 movl $asm_vectors,%esi
 movl $asm_ships,%edi
 movl $asm_clouds,%ebp
 movl $12,%ecx
encounter_find:
 movb 31(%esi),%al
 andb $160,%al
 cmpb $160,%al
 jne encounter_find_next
 cmpw $0,56(%ebp)
 je uni_encounter_ready
encounter_find_next:
 addl $36,%esi
 addl $272,%edi
 addl $448,%ebp
 decl %ecx
 jne encounter_find
 jmp encounter_record_next
.globl uni_encounter_allocate
uni_encounter_allocate:
 movzwl (%ebx),%eax
 xchgb %al,%ah
 call uni_allocate
 testl %eax,%eax
 js encounter_record_next
 imull $36,%eax,%edx
 leal asm_vectors(%edx),%esi
 imull $272,%eax,%edx
 leal asm_ships(%edx),%edi
 imull $448,%eax,%edx
 leal asm_clouds(%edx),%ebp
uni_encounter_ready:
 movw (%ebx),%dx
 xchgb %dl,%dh
 movl $asm_trader,%eax
 cmpw $6,%dx
 je encounter_model_ready
 xorl %ecx,%ecx
encounter_model_loop:
 movl _models(,%ecx,4),%eax
 cmpw 28(%eax),%dx
 je encounter_model_ready
 incl %ecx
 cmpl $10,%ecx
 jb encounter_model_loop
 jmp encounter_record_next
encounter_model_ready:
 movl %eax,(%edi)
 # Copy the exact 36-byte original INWK; native model/cache fields stay native.
 pushl %esi
 pushl %edi
 pushl %eax
 movl %esi,%edi
 leal 2(%ebx),%esi
 movl $36,%ecx
 cld
 rep
 movsb
 popl %eax
 popl %edi
 popl %esi
 movw 24(%eax),%dx
 movb %dl,35(%esi)
 movw 30(%eax),%dx
 andb $7,%dl
 movb %dl,31(%esi)
 movw $0,12(%edi)
 movw $0,14(%edi)
 movb $0,2(%ebp)
 cmpw $7,28(%eax)
 jne encounter_not_station
 leal 32(%esi),%edx
 movl %edx,asm_station
encounter_not_station:
 incl asm_enc_spawns
encounter_record_next:
 addl $40,%ebx
 decw (%esp)
 cmpw $-1,(%esp)
 jne encounter_insert
 addl $4,%esp
encounter_empty:
 popl %ebx
encounter_return:
 ret
.globl asm_station_ai
asm_station_ai:
 pushal
 xorl %ebx,%ebx
 movl $asm_vectors,%ecx
 movl $asm_ships,%edx
 movl $12,%ebp
station_count_police:
 movb 31(%ecx),%al
 andb $160,%al
 cmpb $160,%al
 je station_count_next
 movl (%edx),%eax
 cmpw $2,28(%eax)
 jne station_count_next
 incw %bx
station_count_next:
 addl $36,%ecx
 addl $272,%edx
 decl %ebp
 jne station_count_police
 pushl %ebx
 pushl $asm_laser+5
 pushl %esi
 pushl $asm_enc_output+4
 call _station_police
 addl $16,%esp
 testw %ax,%ax
 je station_ai_restore
 movl asm_enc_spawns,%ebx
 movw $256,asm_enc_output
 movw $512,asm_enc_output+2
 call encounter_emit
 cmpl asm_enc_spawns,%ebx
 je station_ai_restore
 incl asm_police_launches
station_ai_restore:
 popal
 jmp asm_ai_done
