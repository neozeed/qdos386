# Source shell scene helpers. Numeric caches native, INWK raw original bytes.
.text
.globl asm_erase_laser
asm_erase_laser:
 cmpb $0,asm_laser+9
 je asm_erase_laser_done
 cmpw $0,asm_hidden
 jne info_laser_clear
 pushl $asm_laser
 pushl _elite_screen
 call _laser_xor
 addl $8,%esp
info_laser_clear:
 movb $0,asm_laser+9
 movb $0,asm_laser+2
asm_erase_laser_done:
 ret
.globl asm_dust_run
asm_dust_run:
 pushal
 pushl _elite_screen
 pushl 40(%esp)
 movzwl asm_view,%eax
 pushl %eax
 pushl $asm_laser+5
 pushl $asm_params
 pushl $asm_dust
 call _dust_run
 addl $24,%esp
 popal
 ret
.globl asm_draw_dials
asm_draw_dials:
 movzbl asm_ecm,%eax
 pushl %eax
 pushl asm_station
 pushl _elite_screen
 call _dashboard_bulbs
 addl $12,%esp
 movzbl asm_mcnt,%eax
 pushl %eax
 pushl $asm_status
 pushl $asm_controls
 pushl _elite_screen
 call _dials_frame
 addl $16,%esp
 movzbl asm_travel+9,%eax
 pushl %eax
 pushl _elite_screen
 call _dials_fuel
 addl $8,%esp
 movzwl asm_altitude,%eax
 pushl %eax
 pushl _elite_screen
 call _dials_altitude
 addl $8,%esp
 pushl $asm_player
 pushl _elite_screen
 call _missile_icons
 addl $8,%esp
 xorl %edx,%edx
 movl $asm_planet,%eax
 cmpw $3,asm_mode
 jne asm_compass_target_done
 cmpl $0,asm_station
 je asm_compass_target_done
 movl asm_station,%eax
 subl $32,%eax
 movl $1,%edx
asm_compass_target_done:
 pushl %edx
 pushl $asm_compass
 pushl %eax
 pushl _elite_screen
 call _compass_frame
 addl $16,%esp
 ret
.globl asm_reset_vectors
asm_reset_vectors:
 pushl %ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 movb $0,asm_params
 movb $0,asm_params+1
 movb $1,asm_params+2
 movzwl asm_controls+2,%ebx
 movl $asm_controls,%edi
 xorl %eax,%eax
 movl $13,%ecx
 cld
asm_clear_controls:
 rep
 stosb
 movb $128,asm_controls
 movb $128,asm_controls+1
 movb $1,asm_controls+12
 movw %bx,asm_controls+2
 movl $asm_vectors,%esi
 movl $asm_ships,%edi
 movl $11,%ebp
 # EBX retains last model pointer as the source A3 does.
 xorl %ebx,%ebx
asm_reset_vector:
 xorl %ecx,%ecx
asm_zero_vector:
 movb $0,(%esi,%ecx)
 incl %ecx
 cmpl $36,%ecx
 jb asm_zero_vector
 movb $224,14(%esi)
 movb $96,18(%esi)
 movb $96,22(%esi)
 xorl %ecx,%ecx
 xorl %edx,%edx
asm_seed_coordinate:
 movw 4(%edi,%ecx,2),%ax
 pushl %ecx
 xorl %ecx,%ecx
 testw %ax,%ax
 jns asm_seed_positive
 negw %ax
 movl $128,%ecx
asm_seed_positive:
 movb %al,(%esi,%edx)
 movb %ah,1(%esi,%edx)
 movb %cl,2(%esi,%edx)
 popl %ecx
 addl $3,%edx
 incl %ecx
 cmpl $3,%ecx
 jb asm_seed_coordinate
 movb $2,27(%esi)
 movl (%edi),%eax
 testl %eax,%eax
 jz asm_seed_station_done
 movl %eax,%ebx
 movb 24(%ebx),%al
 movb %al,35(%esi)
 cmpw $7,28(%ebx)
 jne asm_seed_energy_done
 leal 32(%esi),%eax
 movl %eax,asm_station
asm_seed_energy_done:
 cmpw $0,22(%ebx)
 jne asm_seed_station_done
 movb $0,27(%esi)
asm_seed_station_done:
 cmpw $2,asm_mode
 jb asm_seed_passive
 movb $240,32(%esi)
 cmpw $3,asm_mode
 jne asm_seed_passive
 cmpw $9,%bp
 jb asm_seed_unused
 movb 30(%ebx),%al
 andb $7,%al
 movb %al,31(%esi)
 jmp asm_seed_passive
asm_seed_unused:
 movb $160,31(%esi)
 movb $0,32(%esi)
asm_seed_passive:
 movb $32,29(%esi)
 movb $16,30(%esi)
 addl $36,%esi
 addl $272,%edi
 decl %ebp
 jns asm_reset_vector
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret
.globl asm_view_label
asm_view_label:
 pushl %esi
 pushl %edi
 movzwl asm_view,%eax
 imull $5,%eax,%eax
 movswl %ax,%eax
 leal asm_view_names(%eax),%esi
 movl $asm_view_text+1,%edi
 movl $5,%ecx
 cld
view_label_copy:
 rep
 movsb
 popl %edi
 popl %esi
 ret
