# RC6 startup, clock, bitmap snapshot and exit sequencing.
# _elite_screen is a process-private packed buffer supplied by the driver.
# Platform functions below are internal adapter boundaries, not new OS APIs.
.text
.globl asm_clock
asm_clock:
 incl asm_ticks
 ret
.globl asm_main
asm_main:
 pushl $0
 pushl $0
 pushl _elite_screen
 call _screen_pack
 addl $12,%esp
 movl $32768,asm_snapshot_size
 movw $0,asm_snapshot_rle
 cmpl $32768,%eax
 jae snapshot_allocate
 movl %eax,asm_snapshot_size
 movw $1,asm_snapshot_rle
snapshot_allocate:
 pushl asm_snapshot_size
 call _platform_alloc
 addl $4,%esp
 # Adapter returns allocated pointer in EAX, error in EDX (source A0/D0).
 testl %edx,%edx
 jz snapshot_allocated
 movl %edx,%eax
 jmp shell_exit
snapshot_allocated:
 movl %eax,asm_snapshot
 cmpw $0,asm_snapshot_rle
 je snapshot_raw
 pushl asm_snapshot_size
 pushl %eax
 pushl _elite_screen
 call _screen_pack
 addl $12,%esp
 testl %eax,%eax
 jns snapshot_saved
 pushl asm_snapshot
 call _platform_free
 addl $4,%esp
 movl $0,asm_snapshot
 movl $-3,%eax
 jmp shell_exit
snapshot_raw:
 movl %eax,%edi
 movl _elite_screen,%esi
 movl $8192,%ecx
 cld
asm_save_screen:
 rep
 movsl
snapshot_saved:
 pushl $asm_olddisplay
 pushl $asm_oldmode
 pushl $-1
 pushl $-1
 call _platform_mode
 addl $16,%esp
 pushl $0
 pushl $0
 pushl $0
 pushl $8
 call _platform_mode
 addl $16,%esp
 movl _elite_screen,%edi
 movl $8192,%ecx
 xorl %eax,%eax
 cld
asm_clear_screen:
 rep
 stosl
 call _asm_labels
 call asm_setup
 cmpw $0,_release_mode
 je startup_ready
 call release_commander
 call flow_remember
 call flow_title_load
startup_ready:
 pushl $asm_clock
 call _platform_poll_install
 addl $4,%esp
 call _platform_free_bytes
 movl %eax,asm_free
 movw $1,asm_ready
 jmp asm_loop
.globl asm_finish
asm_finish:
 movw $0,asm_release_quiet
asm_release_wait:
 call _keyboard_drain
 movw $0,asm_release_keys
 movw $0,asm_release_row
asm_release_scan:
 movzwl asm_release_row,%eax
 pushl %eax
 call asm_keyrow
 addl $4,%esp
 orw %ax,asm_release_keys
 incw asm_release_row
 cmpw $8,asm_release_row
 jl asm_release_scan
 cmpw $0,asm_release_keys
 je asm_release_idle
 movw $0,asm_release_quiet
 jmp asm_release_tick
asm_release_idle:
 incw asm_release_quiet
 cmpw $3,asm_release_quiet
 jge asm_release_done
asm_release_tick:
 pushl $1
 call _platform_yield50
 addl $4,%esp
 jmp asm_release_wait
asm_release_done:
 pushl $asm_sound
 call _sound_stop
 addl $4,%esp
 call _keyboard_drain
 movw $0,asm_ready
 pushl $asm_clock
 call _platform_poll_remove
 addl $4,%esp
 pushl $0
 pushl $0
 movswl asm_olddisplay,%eax
 pushl %eax
 movswl asm_oldmode,%eax
 pushl %eax
 call _platform_mode
 addl $16,%esp
 cmpw $0,asm_snapshot_rle
 je snapshot_restore_raw
 pushl _elite_screen
 pushl asm_snapshot
 call _screen_unpack
 addl $8,%esp
 jmp snapshot_restored
snapshot_restore_raw:
 movl asm_snapshot,%esi
 movl _elite_screen,%edi
 movl $8192,%ecx
 cld
asm_restore_screen:
 rep
 movsl
snapshot_restored:
 pushl asm_snapshot
 call _platform_free
 addl $4,%esp
 movl $0,asm_snapshot
 xorl %eax,%eax
.globl shell_exit
shell_exit:
 pushl %eax
 call _platform_exit
 popl %eax
 jmp shell_exit
# Native cdecl row argument/result. Original IPC packet bytes retained.
.globl asm_keyrow
asm_keyrow:
 movb 4(%esp),%al
 movb %al,asm_ipc+6
 pushl $asm_ipc
 call _platform_keyrow
 addl $4,%esp
 movzbl %al,%eax
 ret
.globl asm_clear_view
asm_clear_view:
 pushl _elite_screen
 call _asm_clear_view
 addl $4,%esp
 ret
.data
.globl _elite_screen
_elite_screen: .long 0

.globl _shell_platform_poll
_shell_platform_poll: .long 0
