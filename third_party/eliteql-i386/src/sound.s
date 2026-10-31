# RC6 sound priority and envelope logic. State byte layout stays big-endian.
# IPC output is a real external boundary: _platform_sound_ipc(packet).
.text
.globl _sound_event
_sound_event:
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 16(%esp),%esi
 movzwl 20(%esp),%eax
 cmpw $72,%ax
 ja sound_reject
 testw $7,%ax
 jnz sound_reject
 shrl $1,%eax
 leal sound_effects(%eax),%edi
 movzbl (%edi),%ecx
 andl $1,%ecx
 shrl $2,%eax
 movb sound_priorities(%eax),%dl
 cmpb (%esi,%ecx),%dl
 jb sound_reject
 movb %dl,(%esi,%ecx)
 andb $15,%dl
 movb %dl,2(%esi,%ecx)
 movb $1,4(%esi)
 movb %al,5(%esi)
 movb %cl,6(%esi)
 movb 1(%edi),%al
 movb %al,7(%esi)
 movb 2(%edi),%al
 movb %al,8(%esi)
 movb %al,12(%esi)
 movb $0,9(%esi)
 movb $0,10(%esi)
 movb $0,11(%esi)
 movzbl 3(%edi),%eax
 leal (%eax,%eax,4),%eax
 xchgb %al,%ah
 movw %ax,14(%esi)
 movl 24(%esp),%eax
 # Byte swap without BSWAP (not available on 80386).
 xchgb %al,%ah
 roll $16,%eax
 xchgb %al,%ah
 movl %eax,16(%esi)
 movw $-1,20(%esi)
 movzbl 1(%edi),%eax
 testl %eax,%eax
 jz sound_silence
 cmpl $4,%eax
 ja sound_accept
 movb %al,9(%esi)
 decl %eax
 imull $6,%eax,%eax
 movb sound_envelopes+3(%eax),%al
 movb %al,11(%esi)
 jmp sound_accept
sound_silence:
 movb $0,4(%esi)
sound_accept:
 movl $1,%eax
 jmp sound_event_done
sound_reject:
 xorl %eax,%eax
sound_event_done:
 popl %edi
 popl %esi
 popl %ebx
 ret
.globl _sound_tick
_sound_tick:
 movl 4(%esp),%ecx
 movl $1,%edx
sound_priority_tick:
 decb 2(%ecx,%edx)
 jns sound_priority_next
 movb $0,2(%ecx,%edx)
 movb $0,(%ecx,%edx)
sound_priority_next:
 decl %edx
 jns sound_priority_tick
 ret
.globl _sound_advance
_sound_advance:
 pushl %ebx
 pushl %esi
 pushl %edi
 movl 16(%esp),%esi
 movl 20(%esp),%edi
sound_step:
 testl %edi,%edi
 jz sound_advance_done
 cmpb $0,4(%esi)
 je sound_advance_done
 decl %edi
 movw 14(%esi),%ax
 xchgb %al,%ah
 testw %ax,%ax
 jz sound_expired
 decw %ax
 pushfl
 xchgb %al,%ah
 movw %ax,14(%esi)
 popfl
 jz sound_expired
 movzbl 9(%esi),%eax
 testl %eax,%eax
 jz sound_step
 decl %eax
 imull $6,%eax,%eax
 leal sound_envelopes(%eax),%ebx
 movzbl 10(%esi),%ecx
 movb (%ebx,%ecx),%al
 addb %al,8(%esi)
 decb 11(%esi)
 jnz sound_step
 incb 10(%esi)
 cmpb $3,10(%esi)
 jb sound_stage
 movb $0,10(%esi)
 movb 12(%esi),%al
 movb %al,8(%esi)
sound_stage:
 movzbl 10(%esi),%ecx
 movb 3(%ebx,%ecx),%al
 movb %al,11(%esi)
 jmp sound_step
sound_expired:
 movb $0,4(%esi)
sound_advance_done:
 popl %edi
 popl %esi
 popl %ebx
 ret
.globl _sound_poll
_sound_poll:
 pushl %esi
 movl 8(%esp),%esi
 movl 12(%esp),%edx
 movl 16(%esi),%eax
 xchgb %al,%ah
 roll $16,%eax
 xchgb %al,%ah
 subl %edx,%eax
 negl %eax
 addl %eax,%eax
 pushl %eax
 movl %edx,%eax
 xchgb %al,%ah
 roll $16,%eax
 xchgb %al,%ah
 movl %eax,16(%esi)
 pushl %esi
 call _sound_advance
 addl $8,%esp
 cmpb $0,4(%esi)
 je sound_poll_silent
 movzbl 8(%esi),%eax
 movzbl sound_pitch(%eax),%eax
 xchgb %al,%ah
 cmpw 20(%esi),%ax
 je sound_poll_done
 movw %ax,20(%esi)
 movb %ah,sound_ipc+6
 movw $256,22(%esi)
 pushl $sound_ipc
 call _platform_sound_ipc
 addl $4,%esp
 jmp sound_poll_done
sound_poll_silent:
 cmpw $0,22(%esi)
 je sound_poll_done
 movw $0,22(%esi)
 pushl $sound_stop_ipc
 call _platform_sound_ipc
 addl $4,%esp
sound_poll_done:
 popl %esi
 ret
.globl _sound_stop
_sound_stop:
 movl 4(%esp),%eax
 movb $0,4(%eax)
 movw $0,22(%eax)
 pushl $sound_stop_ipc
 call _platform_sound_ipc
 addl $4,%esp
 ret
.data

sound_effects:
 .byte 17,1,0,3,17,2,44,4,17,3,240,6,16,241,4,5
 .byte 1,241,188,1,17,244,12,8,16,241,4,6,16,2,96,16
 .byte 17,4,194,255,17,0,0,0
sound_priorities: .byte 112,240,42,162,162,134,133,169,135,57
sound_envelopes:
 .byte 0,111,248,4,1,8,14,238,255,44,32,50
 .byte 1,255,253,17,32,128,4,248,44,4,6,8


sound_pitch:
 .byte 56,55,55,54,53,52,52,51,50,49,49,48,47,47,46,45
 .byte 45,44,43,43,42,42,41,40,40,39,39,38,38,37,36,36
 .byte 35,35,34,34,33,33,32,32,32,31,31,30,30,29,29,29
 .byte 28,28,27,27,27,26,26,25,25,25,24,24,24,23,23,23
 .byte 22,22,22,21,21,21,20,20,20,20,19,19,19,18,18,18
 .byte 18,17,17,17,17,16,16,16,16,16,15,15,15,15,14,14
 .byte 14,14,14,13,13,13,13,13,13,12,12,12,12,12,11,11
 .byte 11,11,11,11,11,10,10,10,10,10,10,10,9,9,9,9
 .byte 9,9,9,8,8,8,8,8,8,8,8,8,7,7,7,7
 .byte 7,7,7,7,7,7,6,6,6,6,6,6,6,6,6,6
 .byte 6,6,5,5,5,5,5,5,5,5,5,5,5,5,5,4
 .byte 4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4
 .byte 4,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3
 .byte 3,3,3,3,3,3,3,3,2,2,2,2,2,2,2,2
 .byte 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
 .byte 2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1
.align 2

sound_ipc: .byte 10,16,68,68,170,102,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0
sound_stop_ipc: .byte 11,0,0,0,0,0,1,0
