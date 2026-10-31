# RC6 missile.s: hostile proximity/ECM, child initialization, ECM countdown.
.text
.globl _hostile_missile
_hostile_missile:
 movl 4(%esp),%edx
 movl 8(%esp),%ecx
 cmpb $0,4(%ecx)
 jne missile_ecm
 movb 1(%edx),%al
 orb 4(%edx),%al
 orb 7(%edx),%al
 je missile_impact
 jmp _missile_steer
missile_ecm:
 orb $128,31(%edx)
 movb (%edx),%al
 orb 3(%edx),%al
 orb 6(%edx),%al
 je missile_blast
 movl $6,%eax
 ret
missile_blast:
 movl $80,%eax
 jmp missile_damage
missile_impact:
 orb $128,31(%edx)
 movl $250,%eax
missile_damage:
 movl 16(%esp),%ecx
 movzbl 8(%edx),%edx
 pushl $1
 pushl %edx
 pushl %eax
 pushl %ecx
 call _player_damage
 addl $16,%esp
 addl $3,%eax
 ret
.globl _spawn_child
_spawn_child:
 pushl %esi
 pushl %edi
 movl 12(%esp),%esi
 movl 16(%esp),%edi
 movl $35,%ecx
child_copy:
 movb (%esi,%ecx),%al
 movb %al,(%edi,%ecx)
 decl %ecx
 jns child_copy
 movl 20(%esp),%esi
 movb $254,32(%edi)
 andb $254,29(%edi)
 # Model blueprint words follow the native i386 short-array ABI.
 movb 24(%esi),%al
 movb %al,35(%edi)
 movb 30(%esi),%al
 andb $7,%al
 movb %al,31(%edi)
 movb $0,33(%edi)
 movb $0,34(%edi)
 popl %edi
 popl %esi
 ret
.globl _ecm_step
_ecm_step:
 movl 4(%esp),%edx
 movl 8(%esp),%ecx
 cmpb $0,1(%edx)
 je ecm_count
 subb $1,2(%ecx)
 jne ecm_count
 addb $1,2(%ecx)
 jmp ecm_off
ecm_count:
 cmpb $0,(%edx)
 je ecm_done
 subb $2,(%edx)
 jne ecm_done
ecm_off:
 movb $0,(%edx)
 movb $0,1(%edx)
ecm_done:
 ret
