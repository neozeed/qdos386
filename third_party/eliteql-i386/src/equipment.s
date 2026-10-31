# RC6 equipment.s, OUCH and energy-bomb state only.
.text
.globl _equipment_damage
_equipment_damage:
 pushl %ebx
 pushl %esi
 movl 12(%esp),%esi
 movl 20(%esp),%ebx
 pushl 24(%esp)
 pushl 20(%esp)
 call _elite_random
 addl $8,%esp
 testb %al,%al
 js equipment_no_loss
 shrl $16,%eax
 andl $255,%eax
 cmpw $22,%ax
 jae equipment_no_loss
 cmpb $0,(%esi,%eax)
 je equipment_no_loss
 testb %bl,%bl
 jne equipment_no_loss
 movb $0,(%esi,%eax)
 incl %eax
 jmp equipment_damage_done
equipment_no_loss:
 xorl %eax,%eax
equipment_damage_done:
 popl %esi
 popl %ebx
 ret
.globl _bomb_key
_bomb_key:
 movl 4(%esp),%edx
 movl 8(%esp),%eax
 testb %al,%al
 je bomb_key_done
 movb (%edx),%al
 addb %al,%al
 movb %al,(%edx)
bomb_key_done:
 ret
.globl _bomb_ship
_bomb_ship:
 movl 4(%esp),%edx
 movl 12(%esp),%eax
 testb %al,%al
 jns bomb_spared
 movl 8(%esp),%eax
 cmpb $7,%al
 je bomb_spared
 testb $32,31(%edx)
 jne bomb_spared
 orb $128,31(%edx)
 movl $1,%eax
 ret
bomb_spared:
 xorl %eax,%eax
 ret
.globl _bomb_tick
_bomb_tick:
 movl 4(%esp),%edx
 cmpb $0,(%edx)
 jns bomb_tick_done
 movb (%edx),%al
 addb %al,%al
 movb %al,(%edx)
bomb_tick_done:
 ret
