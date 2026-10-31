# Original RC6 scene initialization, native cdecl shell helpers.
.text
.globl asm_setup
asm_setup:
 pushl %ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 movw $0,asm_hidden
 movw $0,uni_active
 movl $0,_encounter_pool
 cmpw $0,asm_dust_valid
 je dust_setup_clean
 pushl $3
 call asm_dust_run
 addl $4,%esp
dust_setup_clean:
 movw $0,asm_dust_valid
 movw $0,asm_shop_view
 movl $0,asm_computer_docks
 movw $0,asm_view
 movl $0,asm_police_launches
 call asm_view_label
 pushl $asm_planet_cache
 pushl _elite_screen
 call _render_cache
 addl $8,%esp
 movw $0,asm_planet_cache+12
 movw $255,asm_altitude
 movw $0,asm_chart
 movw $0,asm_local_chart
 movw $0,asm_shop
 movw $0,asm_docked
 movw $0,asm_market_keys
 movw $0,asm_market_selection
 movw $0,asm_market_result
 cmpw $0,asm_keep_launch
 jne asm_market_kept
 movb $22,_ship_fit
 movb $15,_ship_fit+1
 movw $0,_ship_fit+2
 movl $0,_ship_fit+4
 pushl $asm_travel
 call _travel_init
 addl $4,%esp
 movw $0,asm_nav
 movw $0,asm_nav_keys
 movw $0,asm_ambient
 movw $0,asm_world
 movl $0,asm_warps
 movw $0,asm_warp_result
 movw $0,asm_warp_timer
 movl $0,asm_enc_spawns
 movb $0,asm_enc_params+5
 pushl $0
 pushl $5
 pushl $asm_market
 call _market_init
 addl $12,%esp
asm_market_kept:
 movl $asm_clouds,%esi
 movl $12,%ebx
asm_cloud_reset_loop:
 movzwl 56(%esi),%eax
 pushl %eax
 leal 58(%esi),%eax
 pushl %eax
 pushl _elite_screen
 call _cloud_xor
 addl $12,%esp
 addl $448,%esi
 decl %ebx
 jnz asm_cloud_reset_loop
 movl $asm_clouds,%edi
 movl $1344,%ecx
 xorl %eax,%eax
 cld
asm_cloud_zero:
 rep
 stosl
 movl $asm_cargo,%edi
 movl $22,%ecx
asm_cargo_zero:
 rep
 stosb
 movb $255,asm_equipment
 movb $255,asm_equipment+1
 movb $127,asm_equipment+2
 movl $0,asm_losses
 movw $0,asm_bomb_held
 movl $0,asm_compass
 movl $0xe8030000,asm_commander
 movl $0,asm_commander+4
 movw $0,asm_commander+8
 movl $0,asm_scooped
 movl $0,asm_debris
 movl $0,asm_full
 movl $0,asm_pods
 movw $6,asm_spawn_kind
 movb $0,_tactics_cnt
 movl $asm_planet,%edi
 movl $36,%ecx
asm_planet_zero:
 rep
 stosb
 movb $1,asm_planet+2
 movb $4,asm_planet+8
 movb $4,asm_player
 movb $0,asm_player+1
 movb $255,asm_player+2
 movb $0,asm_player+3
 movw $0,asm_ecm
 movl $0,asm_launches
 movw $0,asm_dead
 movl $0,asm_ai_calls
 movl $0,asm_ai_events
 call asm_erase_laser
 cmpb $0,_ship_fit+1
 je asm_sight_ready
 cmpb $0,asm_sight
 jne asm_sight_ready
 pushl _elite_screen
 call _sight_xor
 addl $4,%esp
 movb $1,asm_sight
asm_sight_ready:
 movb $0,asm_laser
 movb $0,asm_laser+1
 movb $0,asm_laser+2
 movl $0,asm_hits
 movl $0,asm_kills
 movl $0,asm_station
 pushl _elite_screen
 call _scanner_reset
 addl $4,%esp
 movl $asm_ships,%esi
 movl $12,%ebx
asm_erase_all:
 movw $0,14(%esi)
 movl $0,(%esi)
 pushl %esi
 pushl _elite_screen
 call _render_ship
 addl $8,%esp
 addl $272,%esi
 decl %ebx
 jnz asm_erase_all
 movl $asm_ships,%esi
 movl $asm_fleet_positions,%edi
 xorl %ebx,%ebx
asm_setup_loop:
 movl %ebx,%eax
 cmpw $10,%ax
 jl asm_setup_model
 subw $10,%ax
asm_setup_model:
 movl _models(,%eax,4),%eax
 movl %eax,(%esi)
 movw (%edi),%ax
 movw %ax,4(%esi)
 movw 2(%edi),%ax
 movw %ax,6(%esi)
 addl $4,%edi
 movw $2400,8(%esi)
 imull $21,%ebx,%eax
 addw $128,%ax
 andw $255,%ax
 movw %ax,10(%esi)
 addl $272,%esi
 incl %ebx
 cmpw asm_count,%bx
 jl asm_setup_loop
 cmpw $1,asm_count
 jne asm_setup_done
 movzwl asm_model,%eax
 movl _models(,%eax,4),%eax
 movl %eax,asm_ships
 movw $0,asm_ships+4
 movw $0,asm_ships+6
 movw $640,asm_ships+8
asm_setup_done:
 cmpw $2,asm_mode
 jb asm_encounter_ready
 movw $-160,asm_ships+4
 movw $0,asm_ships+6
 movw $1400,asm_ships+8
 movw $160,asm_ships+276
 movw $0,asm_ships+278
 movw $1400,asm_ships+280
 movw $0,asm_ships+548
 movw $-160,asm_ships+550
 movw $1400,asm_ships+552
asm_encounter_ready:
 cmpw $3,asm_mode
 jne asm_salvage_models_done
 cmpw $0,asm_salvage
 je asm_salvage_models_done
 movl _models+12,%eax
 movl %eax,asm_ships
 movl _models+16,%eax
 movl %eax,asm_ships+272
asm_salvage_models_done:
 cmpw $3,asm_mode
 jne asm_port_models_done
 cmpw $2,asm_salvage
 jne asm_port_models_done
 movl _models+20,%eax
 movl %eax,asm_ships
 movl _models+12,%eax
 movl %eax,asm_ships+272
 movw $0,asm_ships+4
 movw $0,asm_ships+6
 movw $1200,asm_ships+8
 movw $-500,asm_ships+276
 movw $2600,asm_ships+280
 movw $500,asm_ships+548
 movw $2600,asm_ships+552
asm_port_models_done:
 call asm_reset_vectors
 cmpw $3,asm_mode
 jne asm_port_pose_done
 cmpw $2,asm_salvage
 jne asm_port_pose_done
 movl $asm_vectors+32,asm_station
 movb $0,asm_vectors+27
 movb $0,asm_vectors+29
 movb $0,asm_vectors+30
 movb $0,asm_vectors+32
 movb $0,asm_vectors+18
 movb $0,asm_vectors+22
 movb $96,asm_vectors+16
 movb $224,asm_vectors+24
asm_port_pose_done:
 cmpw $3,asm_mode
 jne asm_not_deep_setup
 cmpw $3,asm_salvage
 jne asm_not_deep_setup
 movl $asm_vectors,%esi
 movl $12,%ecx
asm_deep_clear:
 movb $160,31(%esi)
 movb $0,32(%esi)
 addl $36,%esi
 decl %ecx
 jnz asm_deep_clear
 movl $0,asm_station
asm_not_deep_setup:
 pushl $0
 call asm_dust_run
 addl $4,%esp
 movw $1,asm_dust_valid
 movb $255,asm_status
 movb $255,asm_status+1
 movb $255,asm_status+2
 movb $0,asm_status+3
 movb $0,asm_status+4
 movb $0,asm_mcnt
 call asm_draw_dials
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
