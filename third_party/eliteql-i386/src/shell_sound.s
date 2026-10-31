# Source shell sound orchestration. Internal event id in EAX; all registers
# are preserved where the original used MOVEM. Packet logic remains R2 sound.s.
.text
.globl asm_sound_event
asm_sound_event:
 cmpb $0,flow_config
 jne flow_sound_muted
 pushal
 pushl asm_ticks
 pushl %eax
 pushl $asm_sound
 call _sound_event
 leal 12(%esp),%esp
 movw %ax,asm_sound_carry
 call asm_sound_poll
 popal
 ret
flow_sound_muted:
 movw $0,asm_sound_carry
 ret
.globl asm_sound_poll
asm_sound_poll:
 pushal
 cmpl $0,_shell_platform_poll
 je shell_poll_ready
 call *_shell_platform_poll
shell_poll_ready:
 pushl asm_ticks
 pushl $asm_sound
 call _sound_poll
 leal 8(%esp),%esp
 popal
 ret
.globl asm_sound_hit
asm_sound_hit:
 pushl %eax
 movl $24,%eax
 call asm_sound_event
 movl $16,%eax
 call asm_sound_event
 popl %eax
 ret
.globl asm_ecm_sound
asm_ecm_sound:
 xorl %eax,%eax
 cmpb $0,asm_ecm
 je ecm_sound_value
 movl $255,%eax
ecm_sound_value:
 cmpb asm_sound_ecm,%al
 je ecm_sound_done
 movb %al,asm_sound_ecm
 testb %al,%al
 je ecm_sound_off
 movl $64,%eax
 jmp ecm_sound_send
ecm_sound_off:
 movl $72,%eax
ecm_sound_send:
 call asm_sound_event
ecm_sound_done:
 ret
.globl asm_sound_explosion
asm_sound_explosion:
 pushl %eax
 pushl %ecx
 movl (%edi),%ecx
 cmpw $8,28(%ecx)
 je sound_missile_explosion
 call asm_sound_hit
 jmp sound_explosion_done
sound_missile_explosion:
 movl $24,%eax
 call asm_sound_event
sound_explosion_done:
 popl %ecx
 popl %eax
 ret
.globl asm_sound_contact
asm_sound_contact:
 pushl %eax
 cmpw $0,asm_dead
 jne sound_contact_done
 movl $24,%eax
 call asm_sound_event
sound_contact_done:
 popl %eax
 ret
