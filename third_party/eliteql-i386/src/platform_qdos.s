# KCTX-FIX2 existing public int 0x30 ABI. No kernel changes.
# Thin native platform boundary; all game state and sequencing stay in shell.
.text
.globl _start
_start:
 pushl %ebp
 movl %esp,%ebp
 movl %ebp,platform_entry
 movl $platform_screen,_elite_screen
 movl $_platform_poll,_shell_platform_poll
 # Require the already shipped R13A private logical surface API.
 movl $59,%eax
 xorl %ebx,%ebx
 int $0x30
 testl %eax,%eax
 js platform_start_fail
 testl $16,%eax
 jz platform_unsupported
 movl $3,%ebx
 movl $0x01000100,%ecx
 movl $1,%edx
 movl $59,%eax
 int $0x30
 testl %eax,%eax
 js platform_start_fail
 movl $1,platform_open
 movl $30,%ebx
 xorl %ecx,%ecx
 xorl %edx,%edx
 movl $59,%eax
 int $0x30
 testl %eax,%eax
 js platform_start_fail
 movl $8,%ebx
 movl $0x80000,%ecx
 movl $platform_palette,%edx
 movl $59,%eax
 int $0x30
 testl %eax,%eax
 js platform_start_fail
 movl $31,%ebx
 movl $1,%ecx
 xorl %edx,%edx
 movl $59,%eax
 int $0x30
 testl %eax,%eax
 js platform_start_fail
 movl $20,%eax
 movl $platform_con_name,%ebx
 xorl %ecx,%ecx
 xorl %edx,%edx
 int $0x30
 testl %eax,%eax
 js platform_start_fail
 movl %eax,platform_con
 jmp asm_main
platform_unsupported:
 movl $-19,%eax
platform_start_fail:
 pushl %eax
 call _platform_exit
.globl _platform_alloc
_platform_alloc:
 pushl %ebx
 movl 8(%esp),%ebx
 movl $16,%eax
 int $0x30
 xorl %edx,%edx
 testl %eax,%eax
 jg platform_alloc_ok
 movl %eax,%edx
 movl $0,%eax
platform_alloc_ok:
 popl %ebx
 ret
.globl _platform_free
_platform_free:
 pushl %ebx
 movl 8(%esp),%ebx
 movl $17,%eax
 int $0x30
 popl %ebx
 ret
.globl _platform_free_bytes
_platform_free_bytes:
 movl $18,%eax
 int $0x30
 ret
.globl _platform_mode
_platform_mode:
 # MODE now belongs to our private QL-format buffer; the caller's console
 # and display mode are preserved by the OS surface lifecycle.
 movl 12(%esp),%eax
 testl %eax,%eax
 jz platform_mode_set
 movw $8,(%eax)
 movl 16(%esp),%eax
 testl %eax,%eax
 jz platform_mode_set
 movw $0,(%eax)
platform_mode_set:
 xorl %eax,%eax
 ret
.globl _platform_poll_install
_platform_poll_install:
 movl $32,%eax
 int $0x30
 movl %eax,platform_last_tick
 movl %eax,platform_last_frame
 movl $0,platform_phase
 movl $1,platform_clock_on
 xorl %eax,%eax
 ret
.globl _platform_poll_remove
_platform_poll_remove:
 movl $0,platform_clock_on
 xorl %eax,%eax
 ret
.globl _platform_poll
_platform_poll:
 pushal
 cmpl $0,platform_clock_on
 je platform_poll_done
 movl $32,%eax
 int $0x30
 movl %eax,%ebx
 subl platform_last_tick,%eax
 movl %ebx,platform_last_tick
 movl $596600,%ecx
 mull %ecx
 addl platform_phase,%eax
 adcl $0,%edx
 movl $1193182,%ecx
 divl %ecx
 movl %edx,platform_phase
 addl %eax,asm_ticks
 subl platform_last_frame,%ebx
 cmpl $2,%ebx
 jb platform_poll_done
 movl platform_last_tick,%eax
 movl %eax,platform_last_frame
 call platform_present
platform_poll_done:
 popal
 ret
.globl _platform_yield50
_platform_yield50:
 pushl %ebx
 movl 8(%esp),%ebx
 addl %ebx,%ebx
 # Suspend optional wake flag and job selector must be null (SDK qd_yield).
 xorl %ecx,%ecx
 xorl %edx,%edx
 movl $8,%eax
 int $0x30
 popl %ebx
 call _platform_poll
 ret
platform_present:
 cmpl $0,platform_open
 je platform_present_done
 movl _elite_screen,%esi
 movl $platform_pixels,%edi
 movl $16384,%ecx
platform_pixels_loop:
 movzbl (%esi),%eax
 movl platform_green(,%eax,4),%ebx
 movzbl 1(%esi),%eax
 orl platform_redblue(,%eax,4),%ebx
 movl %ebx,(%edi)
 addl $2,%esi
 addl $4,%edi
 decl %ecx
 jne platform_pixels_loop
 movl $59,%eax
 movl $6,%ebx
 movl $platform_rect,%ecx
 xorl %edx,%edx
 int $0x30
 testl %eax,%eax
 js platform_present_error
 movl $59,%eax
 movl $20,%ebx
 xorl %ecx,%ecx
 xorl %edx,%edx
 int $0x30
platform_present_error:
 movl %eax,platform_last_present
platform_present_done:
 ret
.globl _platform_keyrow
_platform_keyrow:
 pushl %ebx
 pushl %esi
 pushl %edi
 pushl %ebp
 movl 20(%esp),%ebp
 # Each read drains a bounded batch; make and synthetic break events update
 # independent held scan codes before rebuilding the original QL matrix.
 movl $59,%eax
 movl $32,%ebx
 movl $platform_events,%ecx
 movl $64,%edx
 int $0x30
 testl %eax,%eax
 jle platform_key_build
 movl %eax,%edi
 movl $platform_events,%esi
platform_event_loop:
 movl (%esi),%eax
 movl %eax,%edx
 andl $511,%eax
 shrl $9,%edx
 andb $1,%dl
 movb %dl,platform_held(%eax)
 addl $4,%esi
 decl %edi
 jne platform_event_loop
platform_key_build:
 movl $0,platform_rows
 movl $0,platform_rows+4
 xorl %esi,%esi
platform_scan_loop:
 cmpb $0,platform_held(%esi)
 je platform_scan_next
 movzbl platform_keymap(%esi),%eax
 cmpb $255,%al
 je platform_scan_next
 movl %eax,%edx
 shrl $3,%eax
 andl $7,%edx
 movl %edx,%ecx
 movl $1,%edx
 shll %cl,%edx
 orb %dl,platform_rows(%eax)
platform_scan_next:
 incl %esi
 cmpl $512,%esi
 jb platform_scan_loop
 movzbl 6(%ebp),%eax
 andl $7,%eax
 movzbl platform_rows(%eax),%eax
 popl %ebp
 popl %edi
 popl %esi
 popl %ebx
 ret
.globl _platform_keyboard_queue_pop
_platform_keyboard_queue_pop:
 pushl %ebx
 movl platform_con,%ebx
 movl $43,%eax
 movl $19,%ecx
 movl $5,%edx
 int $0x30
 testl %eax,%eax
 jle platform_queue_empty
 movl $26,%eax
 movl $platform_keybyte,%ecx
 movl $1,%edx
 int $0x30
 cmpl $1,%eax
 jne platform_queue_error
 xorl %eax,%eax
 jmp platform_queue_error
platform_queue_empty:
 movl $-19,%eax
platform_queue_error:
 popl %ebx
 ret
.globl _platform_file_open
_platform_file_open:
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 16(%esp),%esi
 movzwl (%esi),%ecx
 xchgb %cl,%ch
 cmpl $255,%ecx
 ja platform_filename_long
 addl $2,%esi
 movl $platform_filename,%edi
 cld
 rep
 movsb
 movb $0,(%edi)
 movl $129,%ecx
 cmpl $3,20(%esp)
 jne platform_file_flags
 movl $142,%ecx
platform_file_flags:
 movl $platform_filename,%ebx
 movl $42,%eax
 xorl %edx,%edx
 int $0x30
 testl %eax,%eax
 js platform_file_open_done
 movl 24(%esp),%edx
 movl %eax,(%edx)
 xorl %eax,%eax
 jmp platform_file_open_done
platform_filename_long:
 movl $-15,%eax
platform_file_open_done:
 popl %edi
 popl %esi
 popl %ebx
 ret
.globl _platform_file_transfer
_platform_file_transfer:
 pushl %ebx
 movl 8(%esp),%ebx
 movl 12(%esp),%ecx
 movl 16(%esp),%edx
 movl $26,%eax
 cmpl $0,20(%esp)
 je platform_transfer
 movl $22,%eax
platform_transfer:
 int $0x30
 movl 24(%esp),%edx
 movl $0,(%edx)
 testl %eax,%eax
 js platform_transfer_done
 movl %eax,(%edx)
 xorl %eax,%eax
platform_transfer_done:
 popl %ebx
 ret
.globl _platform_file_close
_platform_file_close:
 pushl %ebx
 movl 8(%esp),%ebx
 movl $21,%eax
 int $0x30
 popl %ebx
 ret
.globl _platform_sound_ipc
_platform_sound_ipc:
 # KCTX-FIX2 qlcompat explicitly returns ERR_NI for IPC.
 movl $-19,%eax
 ret
.globl _platform_exit
_platform_exit:
 movl 4(%esp),%eax
 movl %eax,platform_status
 cmpl $0,platform_open
 je platform_exit_con
 movl $59,%eax
 movl $4,%ebx
 xorl %ecx,%ecx
 xorl %edx,%edx
 int $0x30
 movl $0,platform_open
platform_exit_con:
 movl platform_con,%ebx
 testl %ebx,%ebx
 jle platform_exit_record
 movl $21,%eax
 int $0x30
platform_exit_record:
 movl platform_status,%ebx
 movl $24,%eax
 int $0x30
 movl platform_entry,%ebp
 leave
 ret
.data
platform_con_name: .ascii "CON\0"
.align 2
platform_palette: .byte 0,0,0, 0,0,255, 255,0,0, 255,0,255, 0,255,0, 0,255,255, 255,255,0, 255,255,255
platform_rect: .long 0,0,256,256,platform_pixels,256
platform_entry: .long 0
platform_con: .long 0
platform_open: .long 0
platform_status: .long 0
platform_clock_on: .long 0
platform_phase: .long 0
platform_last_tick: .long 0
platform_last_frame: .long 0
platform_last_present: .long 0
platform_green:
 .byte 0,0,0,0
 .byte 0,0,0,0
 .byte 0,0,0,4
 .byte 0,0,0,4
 .byte 0,0,0,0
 .byte 0,0,0,0
 .byte 0,0,0,4
 .byte 0,0,0,4
 .byte 0,0,4,0
 .byte 0,0,4,0
 .byte 0,0,4,4
 .byte 0,0,4,4
 .byte 0,0,4,0
 .byte 0,0,4,0
 .byte 0,0,4,4
 .byte 0,0,4,4
 .byte 0,0,0,0
 .byte 0,0,0,0
 .byte 0,0,0,4
 .byte 0,0,0,4
 .byte 0,0,0,0
 .byte 0,0,0,0
 .byte 0,0,0,4
 .byte 0,0,0,4
 .byte 0,0,4,0
 .byte 0,0,4,0
 .byte 0,0,4,4
 .byte 0,0,4,4
 .byte 0,0,4,0
 .byte 0,0,4,0
 .byte 0,0,4,4
 .byte 0,0,4,4
 .byte 0,4,0,0
 .byte 0,4,0,0
 .byte 0,4,0,4
 .byte 0,4,0,4
 .byte 0,4,0,0
 .byte 0,4,0,0
 .byte 0,4,0,4
 .byte 0,4,0,4
 .byte 0,4,4,0
 .byte 0,4,4,0
 .byte 0,4,4,4
 .byte 0,4,4,4
 .byte 0,4,4,0
 .byte 0,4,4,0
 .byte 0,4,4,4
 .byte 0,4,4,4
 .byte 0,4,0,0
 .byte 0,4,0,0
 .byte 0,4,0,4
 .byte 0,4,0,4
 .byte 0,4,0,0
 .byte 0,4,0,0
 .byte 0,4,0,4
 .byte 0,4,0,4
 .byte 0,4,4,0
 .byte 0,4,4,0
 .byte 0,4,4,4
 .byte 0,4,4,4
 .byte 0,4,4,0
 .byte 0,4,4,0
 .byte 0,4,4,4
 .byte 0,4,4,4
 .byte 0,0,0,0
 .byte 0,0,0,0
 .byte 0,0,0,4
 .byte 0,0,0,4
 .byte 0,0,0,0
 .byte 0,0,0,0
 .byte 0,0,0,4
 .byte 0,0,0,4
 .byte 0,0,4,0
 .byte 0,0,4,0
 .byte 0,0,4,4
 .byte 0,0,4,4
 .byte 0,0,4,0
 .byte 0,0,4,0
 .byte 0,0,4,4
 .byte 0,0,4,4
 .byte 0,0,0,0
 .byte 0,0,0,0
 .byte 0,0,0,4
 .byte 0,0,0,4
 .byte 0,0,0,0
 .byte 0,0,0,0
 .byte 0,0,0,4
 .byte 0,0,0,4
 .byte 0,0,4,0
 .byte 0,0,4,0
 .byte 0,0,4,4
 .byte 0,0,4,4
 .byte 0,0,4,0
 .byte 0,0,4,0
 .byte 0,0,4,4
 .byte 0,0,4,4
 .byte 0,4,0,0
 .byte 0,4,0,0
 .byte 0,4,0,4
 .byte 0,4,0,4
 .byte 0,4,0,0
 .byte 0,4,0,0
 .byte 0,4,0,4
 .byte 0,4,0,4
 .byte 0,4,4,0
 .byte 0,4,4,0
 .byte 0,4,4,4
 .byte 0,4,4,4
 .byte 0,4,4,0
 .byte 0,4,4,0
 .byte 0,4,4,4
 .byte 0,4,4,4
 .byte 0,4,0,0
 .byte 0,4,0,0
 .byte 0,4,0,4
 .byte 0,4,0,4
 .byte 0,4,0,0
 .byte 0,4,0,0
 .byte 0,4,0,4
 .byte 0,4,0,4
 .byte 0,4,4,0
 .byte 0,4,4,0
 .byte 0,4,4,4
 .byte 0,4,4,4
 .byte 0,4,4,0
 .byte 0,4,4,0
 .byte 0,4,4,4
 .byte 0,4,4,4
 .byte 4,0,0,0
 .byte 4,0,0,0
 .byte 4,0,0,4
 .byte 4,0,0,4
 .byte 4,0,0,0
 .byte 4,0,0,0
 .byte 4,0,0,4
 .byte 4,0,0,4
 .byte 4,0,4,0
 .byte 4,0,4,0
 .byte 4,0,4,4
 .byte 4,0,4,4
 .byte 4,0,4,0
 .byte 4,0,4,0
 .byte 4,0,4,4
 .byte 4,0,4,4
 .byte 4,0,0,0
 .byte 4,0,0,0
 .byte 4,0,0,4
 .byte 4,0,0,4
 .byte 4,0,0,0
 .byte 4,0,0,0
 .byte 4,0,0,4
 .byte 4,0,0,4
 .byte 4,0,4,0
 .byte 4,0,4,0
 .byte 4,0,4,4
 .byte 4,0,4,4
 .byte 4,0,4,0
 .byte 4,0,4,0
 .byte 4,0,4,4
 .byte 4,0,4,4
 .byte 4,4,0,0
 .byte 4,4,0,0
 .byte 4,4,0,4
 .byte 4,4,0,4
 .byte 4,4,0,0
 .byte 4,4,0,0
 .byte 4,4,0,4
 .byte 4,4,0,4
 .byte 4,4,4,0
 .byte 4,4,4,0
 .byte 4,4,4,4
 .byte 4,4,4,4
 .byte 4,4,4,0
 .byte 4,4,4,0
 .byte 4,4,4,4
 .byte 4,4,4,4
 .byte 4,4,0,0
 .byte 4,4,0,0
 .byte 4,4,0,4
 .byte 4,4,0,4
 .byte 4,4,0,0
 .byte 4,4,0,0
 .byte 4,4,0,4
 .byte 4,4,0,4
 .byte 4,4,4,0
 .byte 4,4,4,0
 .byte 4,4,4,4
 .byte 4,4,4,4
 .byte 4,4,4,0
 .byte 4,4,4,0
 .byte 4,4,4,4
 .byte 4,4,4,4
 .byte 4,0,0,0
 .byte 4,0,0,0
 .byte 4,0,0,4
 .byte 4,0,0,4
 .byte 4,0,0,0
 .byte 4,0,0,0
 .byte 4,0,0,4
 .byte 4,0,0,4
 .byte 4,0,4,0
 .byte 4,0,4,0
 .byte 4,0,4,4
 .byte 4,0,4,4
 .byte 4,0,4,0
 .byte 4,0,4,0
 .byte 4,0,4,4
 .byte 4,0,4,4
 .byte 4,0,0,0
 .byte 4,0,0,0
 .byte 4,0,0,4
 .byte 4,0,0,4
 .byte 4,0,0,0
 .byte 4,0,0,0
 .byte 4,0,0,4
 .byte 4,0,0,4
 .byte 4,0,4,0
 .byte 4,0,4,0
 .byte 4,0,4,4
 .byte 4,0,4,4
 .byte 4,0,4,0
 .byte 4,0,4,0
 .byte 4,0,4,4
 .byte 4,0,4,4
 .byte 4,4,0,0
 .byte 4,4,0,0
 .byte 4,4,0,4
 .byte 4,4,0,4
 .byte 4,4,0,0
 .byte 4,4,0,0
 .byte 4,4,0,4
 .byte 4,4,0,4
 .byte 4,4,4,0
 .byte 4,4,4,0
 .byte 4,4,4,4
 .byte 4,4,4,4
 .byte 4,4,4,0
 .byte 4,4,4,0
 .byte 4,4,4,4
 .byte 4,4,4,4
 .byte 4,4,0,0
 .byte 4,4,0,0
 .byte 4,4,0,4
 .byte 4,4,0,4
 .byte 4,4,0,0
 .byte 4,4,0,0
 .byte 4,4,0,4
 .byte 4,4,0,4
 .byte 4,4,4,0
 .byte 4,4,4,0
 .byte 4,4,4,4
 .byte 4,4,4,4
 .byte 4,4,4,0
 .byte 4,4,4,0
 .byte 4,4,4,4
 .byte 4,4,4,4
platform_redblue:
 .byte 0,0,0,0
 .byte 0,0,0,1
 .byte 0,0,0,2
 .byte 0,0,0,3
 .byte 0,0,1,0
 .byte 0,0,1,1
 .byte 0,0,1,2
 .byte 0,0,1,3
 .byte 0,0,2,0
 .byte 0,0,2,1
 .byte 0,0,2,2
 .byte 0,0,2,3
 .byte 0,0,3,0
 .byte 0,0,3,1
 .byte 0,0,3,2
 .byte 0,0,3,3
 .byte 0,1,0,0
 .byte 0,1,0,1
 .byte 0,1,0,2
 .byte 0,1,0,3
 .byte 0,1,1,0
 .byte 0,1,1,1
 .byte 0,1,1,2
 .byte 0,1,1,3
 .byte 0,1,2,0
 .byte 0,1,2,1
 .byte 0,1,2,2
 .byte 0,1,2,3
 .byte 0,1,3,0
 .byte 0,1,3,1
 .byte 0,1,3,2
 .byte 0,1,3,3
 .byte 0,2,0,0
 .byte 0,2,0,1
 .byte 0,2,0,2
 .byte 0,2,0,3
 .byte 0,2,1,0
 .byte 0,2,1,1
 .byte 0,2,1,2
 .byte 0,2,1,3
 .byte 0,2,2,0
 .byte 0,2,2,1
 .byte 0,2,2,2
 .byte 0,2,2,3
 .byte 0,2,3,0
 .byte 0,2,3,1
 .byte 0,2,3,2
 .byte 0,2,3,3
 .byte 0,3,0,0
 .byte 0,3,0,1
 .byte 0,3,0,2
 .byte 0,3,0,3
 .byte 0,3,1,0
 .byte 0,3,1,1
 .byte 0,3,1,2
 .byte 0,3,1,3
 .byte 0,3,2,0
 .byte 0,3,2,1
 .byte 0,3,2,2
 .byte 0,3,2,3
 .byte 0,3,3,0
 .byte 0,3,3,1
 .byte 0,3,3,2
 .byte 0,3,3,3
 .byte 1,0,0,0
 .byte 1,0,0,1
 .byte 1,0,0,2
 .byte 1,0,0,3
 .byte 1,0,1,0
 .byte 1,0,1,1
 .byte 1,0,1,2
 .byte 1,0,1,3
 .byte 1,0,2,0
 .byte 1,0,2,1
 .byte 1,0,2,2
 .byte 1,0,2,3
 .byte 1,0,3,0
 .byte 1,0,3,1
 .byte 1,0,3,2
 .byte 1,0,3,3
 .byte 1,1,0,0
 .byte 1,1,0,1
 .byte 1,1,0,2
 .byte 1,1,0,3
 .byte 1,1,1,0
 .byte 1,1,1,1
 .byte 1,1,1,2
 .byte 1,1,1,3
 .byte 1,1,2,0
 .byte 1,1,2,1
 .byte 1,1,2,2
 .byte 1,1,2,3
 .byte 1,1,3,0
 .byte 1,1,3,1
 .byte 1,1,3,2
 .byte 1,1,3,3
 .byte 1,2,0,0
 .byte 1,2,0,1
 .byte 1,2,0,2
 .byte 1,2,0,3
 .byte 1,2,1,0
 .byte 1,2,1,1
 .byte 1,2,1,2
 .byte 1,2,1,3
 .byte 1,2,2,0
 .byte 1,2,2,1
 .byte 1,2,2,2
 .byte 1,2,2,3
 .byte 1,2,3,0
 .byte 1,2,3,1
 .byte 1,2,3,2
 .byte 1,2,3,3
 .byte 1,3,0,0
 .byte 1,3,0,1
 .byte 1,3,0,2
 .byte 1,3,0,3
 .byte 1,3,1,0
 .byte 1,3,1,1
 .byte 1,3,1,2
 .byte 1,3,1,3
 .byte 1,3,2,0
 .byte 1,3,2,1
 .byte 1,3,2,2
 .byte 1,3,2,3
 .byte 1,3,3,0
 .byte 1,3,3,1
 .byte 1,3,3,2
 .byte 1,3,3,3
 .byte 2,0,0,0
 .byte 2,0,0,1
 .byte 2,0,0,2
 .byte 2,0,0,3
 .byte 2,0,1,0
 .byte 2,0,1,1
 .byte 2,0,1,2
 .byte 2,0,1,3
 .byte 2,0,2,0
 .byte 2,0,2,1
 .byte 2,0,2,2
 .byte 2,0,2,3
 .byte 2,0,3,0
 .byte 2,0,3,1
 .byte 2,0,3,2
 .byte 2,0,3,3
 .byte 2,1,0,0
 .byte 2,1,0,1
 .byte 2,1,0,2
 .byte 2,1,0,3
 .byte 2,1,1,0
 .byte 2,1,1,1
 .byte 2,1,1,2
 .byte 2,1,1,3
 .byte 2,1,2,0
 .byte 2,1,2,1
 .byte 2,1,2,2
 .byte 2,1,2,3
 .byte 2,1,3,0
 .byte 2,1,3,1
 .byte 2,1,3,2
 .byte 2,1,3,3
 .byte 2,2,0,0
 .byte 2,2,0,1
 .byte 2,2,0,2
 .byte 2,2,0,3
 .byte 2,2,1,0
 .byte 2,2,1,1
 .byte 2,2,1,2
 .byte 2,2,1,3
 .byte 2,2,2,0
 .byte 2,2,2,1
 .byte 2,2,2,2
 .byte 2,2,2,3
 .byte 2,2,3,0
 .byte 2,2,3,1
 .byte 2,2,3,2
 .byte 2,2,3,3
 .byte 2,3,0,0
 .byte 2,3,0,1
 .byte 2,3,0,2
 .byte 2,3,0,3
 .byte 2,3,1,0
 .byte 2,3,1,1
 .byte 2,3,1,2
 .byte 2,3,1,3
 .byte 2,3,2,0
 .byte 2,3,2,1
 .byte 2,3,2,2
 .byte 2,3,2,3
 .byte 2,3,3,0
 .byte 2,3,3,1
 .byte 2,3,3,2
 .byte 2,3,3,3
 .byte 3,0,0,0
 .byte 3,0,0,1
 .byte 3,0,0,2
 .byte 3,0,0,3
 .byte 3,0,1,0
 .byte 3,0,1,1
 .byte 3,0,1,2
 .byte 3,0,1,3
 .byte 3,0,2,0
 .byte 3,0,2,1
 .byte 3,0,2,2
 .byte 3,0,2,3
 .byte 3,0,3,0
 .byte 3,0,3,1
 .byte 3,0,3,2
 .byte 3,0,3,3
 .byte 3,1,0,0
 .byte 3,1,0,1
 .byte 3,1,0,2
 .byte 3,1,0,3
 .byte 3,1,1,0
 .byte 3,1,1,1
 .byte 3,1,1,2
 .byte 3,1,1,3
 .byte 3,1,2,0
 .byte 3,1,2,1
 .byte 3,1,2,2
 .byte 3,1,2,3
 .byte 3,1,3,0
 .byte 3,1,3,1
 .byte 3,1,3,2
 .byte 3,1,3,3
 .byte 3,2,0,0
 .byte 3,2,0,1
 .byte 3,2,0,2
 .byte 3,2,0,3
 .byte 3,2,1,0
 .byte 3,2,1,1
 .byte 3,2,1,2
 .byte 3,2,1,3
 .byte 3,2,2,0
 .byte 3,2,2,1
 .byte 3,2,2,2
 .byte 3,2,2,3
 .byte 3,2,3,0
 .byte 3,2,3,1
 .byte 3,2,3,2
 .byte 3,2,3,3
 .byte 3,3,0,0
 .byte 3,3,0,1
 .byte 3,3,0,2
 .byte 3,3,0,3
 .byte 3,3,1,0
 .byte 3,3,1,1
 .byte 3,3,1,2
 .byte 3,3,1,3
 .byte 3,3,2,0
 .byte 3,3,2,1
 .byte 3,3,2,2
 .byte 3,3,2,3
 .byte 3,3,3,0
 .byte 3,3,3,1
 .byte 3,3,3,2
 .byte 3,3,3,3
platform_keymap:
 .byte 21,11,35,49,33,6,2,50,7,48,40,53,45,29,9,43
 .byte 51,41,52,44,54,46,55,42,47,37,16,255,8,57,36,27
 .byte 38,28,30,34,39,26,32,31,23,24,56,13,17,59,19,60
 .byte 20,62,22,18,63,61,56,255,58,14,25,1,3,4,0,5
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,57,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,58,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,10,255,255,9,255,12,255,255
 .byte 15,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
 .byte 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
.data
.align 2
platform_screen: .space 32768
platform_pixels: .space 65536
platform_events: .space 256
platform_held: .space 512
platform_rows: .space 8
platform_filename: .space 256
platform_keybyte: .space 4
